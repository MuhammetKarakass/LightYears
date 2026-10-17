#pragma once

#include "abilities/AbilityPolicies.h"

#include <string>
#include <vector>

namespace vega
{
	struct DefaultAbilityLoadoutEntry
	{
		std::string abilityId;
		sas::AbilitySlot slot = sas::AbilitySlot::None;
	};

	const std::vector<DefaultAbilityLoadoutEntry>& GetDefaultAbilityLoadout();
}
