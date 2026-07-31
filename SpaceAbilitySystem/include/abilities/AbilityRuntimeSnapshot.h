#pragma once

#include "abilities/AbilityHandle.h"
#include "abilities/AbilityPolicies.h"

#include <string>

namespace sas
{
	struct AbilityRuntimeSnapshot
	{
		AbilityHandle handle;
		std::string abilityId;
		AbilitySlot slot = AbilitySlot::None;
		int level = 1;
		int maxLevel = 1;
		bool active = false;
		float cooldownRemaining = 0.f;
		float cooldownDuration = 0.f;
		float activeRemaining = 0.f;
		float activeDuration = 0.f;
		int charges = 0;
	};
}
