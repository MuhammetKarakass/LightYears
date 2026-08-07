#include "gameplay/ability/shield/ShieldAbility.h"

#include "gameConfigs/combat/EffectConfig.h"

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
		if (!barrier || !barrierSpec ||
			barrier->durationPolicy != sas::GameplayEffectDurationPolicy::Duration ||
			!barrierSpec->useAbilityDuration ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Shield must own a positive barrier duration through useAbilityDuration.";
			}
			return false;
		}
		for (const GameplayTag& requiredAttribute : {
			BarrierEffectSchema::Capacity,
			BarrierEffectSchema::AbsorptionRatio,
			BarrierEffectSchema::RegenerationPerSecond,
			BarrierEffectSchema::RegenerationDelay,
			BarrierEffectSchema::RegenerationDelayRemaining
		})
		{
			if (!sas::FindGameplayAttribute(barrierSpec->attributes, requiredAttribute))
			{
				if (failureReason)
				{
					*failureReason =
						"Shield ability effectSpecs is missing a required barrier attribute.";
				}
				return false;
			}
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
