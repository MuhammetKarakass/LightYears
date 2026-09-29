#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/dash/DashAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameConfigs/ability/movement/DashConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "gameplay/ability/dash/DashMovementMath.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/Actor.h"

#include <array>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t ExpectedProgressionSteps = 24;
		constexpr float ProgressionTolerance = 0.0001f;

		float ResolveNumericSetting(
			const std::string& abilityId,
			const std::string& settingName,
			float fallback
		)
		{
			return content::AbilityContentCatalog::FindNumericSetting(abilityId, settingName)
				.value_or(fallback);
		}

		float ApplyCooldownStep(float cooldown, const AbilityLevelStep& step)
		{
			float result = cooldown;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId != CommonAttributeIds::Cooldown)
				{
					continue;
				}

				switch (modifier.operation)
				{
				case sas::AttributeModifierOperation::Add:
					result += modifier.magnitude;
					break;
				case sas::AttributeModifierOperation::Multiply:
					result *= modifier.magnitude;
					break;
				case sas::AttributeModifierOperation::Override:
					result = modifier.magnitude;
					break;
				}
			}
			return result;
		}

		bool IsExpectedMagnitude(float value, float expected)
		{
			return std::isfinite(value) && std::abs(value - expected) <= ProgressionTolerance;
		}

		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}
	}

	bool DashAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		const float baseDistance = ResolveNumericSetting(
			definition.abilityId,
			AbilityData::Dash::Setting::BaseDistance,
			0.f
		);
		const float cameraZoomOutRatio = ResolveNumericSetting(
			definition.abilityId,
			AbilityData::Dash::Setting::CameraZoomOutRatio,
			0.f
		);
		const sas::GameplayAttribute* moveSpeedScale = sas::FindAttribute(
			definition.attributes,
			AbilityData::Dash::Attribute::MoveSpeedScale
		);
		if (!std::isfinite(baseDistance) || baseDistance <= 0.f ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!moveSpeedScale || !std::isfinite(moveSpeedScale->baseValue) || moveSpeedScale->baseValue <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f ||
			!std::isfinite(cameraZoomOutRatio) || cameraZoomOutRatio < 0.f || cameraZoomOutRatio > 0.5f ||
			definition.levelProgression.size() != ExpectedProgressionSteps)
		{
			if (failureReason)
			{
				*failureReason =
					"Dash requires positive movement, cooldown, and MoveSpeedScale values, a camera zoom-out ratio from 0 to 0.5, "
					"and twenty-four JSON-owned progression steps.";
			}
			return false;
		}
		if (definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.duration <= 0.f ||
			(content::AbilityContentCatalog::FindById(definition.abilityId) &&
				std::abs(
					definition.duration -
					content::AbilityContentCatalog::FindById(definition.abilityId)->duration
				) > 0.0001f))
		{
			if (failureReason)
			{
				*failureReason = "Dash lifetime duration must match its movement settings.";
			}
			return false;
		}
		float previousCooldown = definition.cooldown;
		for (std::size_t stepIndex = 0; stepIndex < definition.levelProgression.size(); ++stepIndex)
		{
			const AbilityLevelStep& step = definition.levelProgression[stepIndex];
			const float expectedCooldownDelta = -GetGlobalAbilityCooldownStepReduction(
				definition.cooldown,
				stepIndex
			);
			int cooldownModifierCount = 0;
			int moveSpeedModifierCount = 0;
			bool progressionMatches = step.attributeModifiers.size() == 2 &&
				step.unlockedUpgradeIds.empty() && step.addedActions.empty() &&
				step.addedTriggers.empty() && step.scalingRules.empty();
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Cooldown)
				{
					++cooldownModifierCount;
					progressionMatches = progressionMatches &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						IsExpectedMagnitude(modifier.magnitude, expectedCooldownDelta);
				}
				else if (modifier.attributeId == AbilityData::Dash::Attribute::MoveSpeedScale)
				{
					++moveSpeedModifierCount;
					progressionMatches = progressionMatches &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						IsExpectedMagnitude(
							modifier.magnitude,
							AbilityData::Dash::MoveSpeedScaleUpgrade
						);
				}
			}
			if (!progressionMatches || cooldownModifierCount != 1 || moveSpeedModifierCount != 1)
			{
				if (failureReason)
				{
					*failureReason =
						"Basic Dash requires one authored cooldown and MoveSpeedScale addition at each level.";
				}
				return false;
			}

			const float levelCooldown = ApplyCooldownStep(previousCooldown, step);
			if (!std::isfinite(levelCooldown) || levelCooldown < 1.f - ProgressionTolerance ||
				levelCooldown > previousCooldown + ProgressionTolerance)
			{
				if (failureReason)
				{
					*failureReason =
						"Basic Dash cooldown progression must remain positive and decrease per level.";
				}
				return false;
			}
			previousCooldown = levelCooldown;
		}
		return true;
	}

	bool DashAbility::Activate(GameAbilityBehaviorContext& context)
	{
		auto* movementController = dynamic_cast<DashMovementController*>(&context.owner);
		if (!movementController)
		{
			return false;
		}

		const float baseDistance = ResolveNumericSetting(
			context.definition.abilityId,
			AbilityData::Dash::Setting::BaseDistance,
			0.f
		);
		const sf::Vector2f direction = movementController->ResolveDashDirection();
		const sas::AttributeSystem& ownerAttributes = context.abilitySystem.GetAttributes();
		const float horizontalRating = ownerAttributes.HasAttribute(OwnerAttributeIds::MoveSpeedHorizontal)
			? ownerAttributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal)
			: 0.f;
		const float verticalRating = ownerAttributes.HasAttribute(OwnerAttributeIds::MoveSpeedVertical)
			? ownerAttributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical)
			: 0.f;
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float moveSpeedScale = sas::FindAttributeValue(
			values,
			AbilityData::Dash::Attribute::MoveSpeedScale,
			AbilityData::Dash::DefaultMoveSpeedScale
		);
		const float resolvedDistance = DashMovementMath::ResolveDistanceFromMoveSpeed(
			baseDistance,
			direction,
			horizontalRating,
			verticalRating,
			moveSpeedScale
		);
		const DashRequest request{
			direction,
			resolvedDistance,
			context.definition.duration,
			true
		};
		if (!movementController->StartDash(request))
		{
			return false;
		}

		mStarted = true;
		context.abilitySystem.AddOwnedTag(GameplayTags::State::Ability::Dash::Active);
		return true;
	}

	void DashAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason)
	{
		if (!mStarted)
		{
			return;
		}

		if (auto* movementController = dynamic_cast<DashMovementController*>(&context.owner))
		{
			movementController->EndDash();
		}
		context.abilitySystem.RemoveOwnedTag(GameplayTags::State::Ability::Dash::Active);
		mStarted = false;
	}
}
