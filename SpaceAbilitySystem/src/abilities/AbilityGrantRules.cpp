#include "abilities/AbilityGrantRules.h"

namespace sas
{
	bool IsPassiveAbility(const AbilityDefinition& definition)
	{
		return definition.slot == AbilitySlot::None;
	}

	bool ValidateAbilityGrant(
		const AbilityDefinition& definition,
		std::size_t currentPassiveCount,
		std::size_t maxPassiveCount,
		std::string* failureReason
	)
	{
		const bool isPassive = IsPassiveAbility(definition);
		if (isPassive && currentPassiveCount >= maxPassiveCount)
		{
			if (failureReason)
			{
				*failureReason = "Passive ability limit reached.";
			}
			return false;
		}
		if (!isPassive && definition.slot == AbilitySlot::None)
		{
			if (failureReason)
			{
				*failureReason = "Active abilities require an input slot.";
			}
			return false;
		}
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
