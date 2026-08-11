#pragma once

#include "abilities/AbilityEvent.h"
#include "framework/Core.h"

#include <cstddef>
#include <optional>
#include <string>

namespace sas
{
	struct AbilityTrigger
	{
		ly::GameplayTag eventTag;
		float internalCooldown = 0.f;
		ly::List<ly::GameplayTag> requiredTags;
		ly::List<ly::GameplayTag> blockedTags;
		// Empty means any source ability. These filters keep ability identity and
		// end semantics in payload data instead of multiplying event tags.
		ContentId abilityId;
		std::optional<AbilityEndReason> endReason;
		// These filters apply to the source ability carried by a lifecycle event.
		// They intentionally stay separate from owner-state filters above and are
		// appended to preserve existing aggregate content initializers.
		ly::List<ly::GameplayTag> requiredAbilityTags;
		ly::List<ly::GameplayTag> blockedAbilityTags;
		// A zero limit means unlimited matches. consumeOnMatch is the compact
		// one-shot form and takes precedence over maxMatches.
		bool consumeOnMatch = false;
		int maxMatches = 0;
		ly::List<ly::GameplayTag> requiredPayloadTags;
		ly::List<ly::GameplayTag> blockedPayloadTags;
	};

	bool MatchesAbilityTrigger(
		const AbilityEvent& event,
		const AbilityTrigger& trigger,
		const ly::GameplayTagContainer& ownedTags
	);

	bool MatchesAbilityTrigger(
		const AbilityLifecycleEvent& event,
		const AbilityTrigger& trigger,
		const ly::GameplayTagContainer& ownedTags
	);

	std::string MakeAbilityTriggerCooldownKey(
		const std::string& abilityId,
		const AbilityTrigger& trigger,
		std::size_t triggerIndex
	);
}
