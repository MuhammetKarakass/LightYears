#pragma once

#include "abilities/AbilityEvent.h"
#include "framework/Core.h"

#include <cstddef>
#include <string>

namespace sas
{
	struct AbilityTrigger
	{
		ly::GameplayTag eventTag;
		float internalCooldown = 0.f;
		ly::List<ly::GameplayTag> requiredTags;
		ly::List<ly::GameplayTag> blockedTags;
	};

	bool MatchesAbilityTrigger(
		const AbilityEvent& event,
		const AbilityTrigger& trigger,
		const ly::GameplayTagContainer& ownedTags
	);

	std::string MakeAbilityTriggerCooldownKey(
		const std::string& abilityId,
		const AbilityTrigger& trigger,
		std::size_t triggerIndex
	);
}
