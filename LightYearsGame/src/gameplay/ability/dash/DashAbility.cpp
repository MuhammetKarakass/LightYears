#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/dash/DashAbility.h"

#include "gameConfigs/ability/DashConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "framework/Actor.h"

namespace ly
{
	namespace
	{
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
		const AbilityData::Dash::Settings* settings =
			AbilityData::Dash::FindSettings(definition.abilityId);
		if (!settings)
		{
			if (failureReason)
			{
				*failureReason = "Dash behavior references an unknown Dash definition.";
			}
			return false;
		}
		if (settings->baseDistance <= 0.f || settings->duration <= 0.f ||
			settings->cameraZoomOutRatio < 0.f || settings->cameraZoomOutRatio > 0.5f ||
			settings->cooldownReductionPerLevelRatio < 0.f ||
			settings->cooldownReductionPerLevelRatio >= 0.25f)
		{
			if (failureReason)
			{
				*failureReason =
					"Dash settings require positive movement values, a camera zoom-out ratio from 0 to 0.5, "
					"and a per-level cooldown reduction ratio from 0 to below 0.25.";
			}
			return false;
		}
		if (definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.duration != settings->duration)
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
		const AbilityData::Dash::Settings* settings =
			AbilityData::Dash::FindSettings(context.definition.abilityId);
		auto* movementController = dynamic_cast<DashMovementController*>(&context.owner);
		if (!settings || !movementController)
		{
			return false;
		}

		const DashRequest request{
			movementController->ResolveDashDirection(),
			settings->baseDistance,
			settings->duration
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
