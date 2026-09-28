#include "gameplay/ability/shield/ShieldAbility.h"

#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/shield/ShieldContracts.h"

#include <cmath>
#include <variant>

namespace ly
{
	bool ShieldAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		const sas::GameplayEffectDefinition* barrier =
			EffectData::FindGameplayEffectDefinition(BarrierEffectSchema::BasicEffectId);
		const AbilityEffectSpecDefinition* barrierSpec =
			definition.FindEffectSpec(BarrierEffectSchema::BasicEffectId);
		if (definition.abilityId != AbilityData::Shield::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!barrier || !barrierSpec || definition.effectSpecs.size() != 1 ||
			barrier->durationPolicy != sas::GameplayEffectDurationPolicy::Duration ||
			barrier->stackingPolicy != sas::GameplayEffectStackingPolicy::RefreshDuration ||
			barrierSpec->maxStacks != 1 || !definition.triggers.empty() ||
			!barrierSpec->useAbilityDuration ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Shield must own a positive barrier duration through useAbilityDuration.";
			}
			return false;
		}
		for (const sas::AttributeId& requiredAttribute : {
			BarrierEffectSchema::Capacity,
			BarrierEffectSchema::AbsorptionRatio,
			BarrierEffectSchema::RegenerationPerSecond,
			BarrierEffectSchema::RegenerationDelay,
			BarrierEffectSchema::RegenerationDelayRemaining
		})
		{
			if (!sas::FindAttribute(barrierSpec->attributes, requiredAttribute))
			{
				if (failureReason)
				{
					*failureReason =
						"Shield ability effectSpecs is missing a required barrier attribute.";
				}
				return false;
			}
		}
		const auto* capacity = sas::FindAttribute(barrierSpec->attributes, BarrierEffectSchema::Capacity);
		const auto* regeneration = sas::FindAttribute(
			barrierSpec->attributes, BarrierEffectSchema::RegenerationPerSecond);
		if (!capacity || !std::isfinite(capacity->baseValue) || capacity->baseValue <= 0.f ||
			!regeneration || regeneration->baseValue != 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Shield requires positive capacity and no regeneration.";
			}
			return false;
		}

		for (const AbilityActionSpec& action : definition.actions)
		{
			if (action.phase == sas::AbilityActionPhase::OnActivate &&
				std::holds_alternative<ApplyEffectAction>(action.action))
			{
				return true;
			}
		}

		if (failureReason)
		{
			*failureReason = "Shield abilities require an OnActivate effect action.";
		}
		return false;
	}
}
