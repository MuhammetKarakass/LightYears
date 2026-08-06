#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/dash/DashAbility.h"

#include "gameConfigs/ability/functional/DashConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "framework/Actor.h"

#include <cmath>

namespace ly
{
	namespace
	{
		float ResolveNumericSetting(
			const std::string& abilityId,
			const std::string& settingName,
			float fallback
		)
		{
			return content::AbilityContentCatalog::FindNumericSetting(abilityId, settingName)
				.value_or(fallback);
		}

		void EmitLifecycleEvent(
			LightYearsAbilitySystemComponent& abilitySystem,
			Actor& owner,
			const GameplayTag& eventTag)
		{
			sas::AbilityEvent event;
			event.eventTag = eventTag;
			event.SetSource(&owner);
			event.SetTarget(&owner);
			abilitySystem.HandleGameplayEvent(event);
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
	}

	bool DashAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		const float baseDistance = ResolveNumericSetting(
			definition.abilityId,
			"baseDistance",
			0.f
		);
		const float cameraZoomOutRatio = ResolveNumericSetting(
			definition.abilityId,
			"cameraZoomOutRatio",
			0.f
		);
		if (baseDistance <= 0.f || definition.duration <= 0.f ||
			cameraZoomOutRatio < 0.f || cameraZoomOutRatio > 0.5f ||
			definition.levelProgression.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Dash settings require positive movement values, a camera zoom-out ratio from 0 to 0.5, "
					"and a JSON-owned cooldown progression.";
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
		if (definition.levelProgression.size() != 4)
		{
			if (failureReason)
			{
				*failureReason = "Basic Dash requires four cooldown progression steps.";
			}
			return false;
		}

		float previousCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			const float levelCooldown = ApplyCooldownStep(previousCooldown, step);
			if (levelCooldown <= 0.f || levelCooldown >= previousCooldown)
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
			"baseDistance",
			0.f
		);
		const DashRequest request{
			movementController->ResolveDashDirection(),
			baseDistance,
			context.definition.duration
		};
		if (!movementController->StartDash(request))
		{
			return false;
		}

		mStarted = true;
		context.abilitySystem.AddOwnedTag(AbilityData::Dash::StateTag);
		EmitLifecycleEvent(
			context.abilitySystem,
			context.owner,
			AbilityData::Dash::StartEvent
		);
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
		context.abilitySystem.RemoveOwnedTag(AbilityData::Dash::StateTag);
		EmitLifecycleEvent(
			context.abilitySystem,
			context.owner,
			AbilityData::Dash::EndEvent
		);
		mStarted = false;
	}
}
