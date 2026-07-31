#include "abilities/AbilityDefinitionValidation.h"

namespace sas
{
	bool ValidateAbilityDefinition(
		const AbilityDefinition& definition,
		std::string* failureReason
	)
	{
		if (definition.abilityId.empty())
		{
			if (failureReason)
			{
				*failureReason = "Ability definition requires an ID.";
			}
			return false;
		}
		if (definition.cooldown < 0.f || definition.duration < 0.f || definition.maxCharges < 0)
		{
			if (failureReason)
			{
				*failureReason = "Ability cooldown, duration, and charge count must be non-negative.";
			}
			return false;
		}
		if (definition.lifetimePolicy == AbilityLifetimePolicy::Duration && definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Duration abilities require a positive duration.";
			}
			return false;
		}

		const bool isPassive = definition.slot == AbilitySlot::None;
		if (isPassive &&
			definition.activationPolicy == AbilityActivationPolicy::Passive &&
			definition.lifetimePolicy != AbilityLifetimePolicy::UntilCancelled)
		{
			if (failureReason)
			{
				*failureReason = "Passive abilities must use UntilCancelled lifetime.";
			}
			return false;
		}
		return true;
	}
}
