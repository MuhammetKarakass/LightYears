#include "abilities/AbilityTrigger.h"

namespace sas
{
	bool MatchesAbilityTrigger(
		const AbilityEvent& event,
		const AbilityTrigger& trigger,
		const ly::GameplayTagContainer& ownedTags
	)
	{
		return event.eventTag.MatchesTag(trigger.eventTag) &&
			ownedTags.HasAll(trigger.requiredTags) &&
			!ownedTags.HasAny(trigger.blockedTags);
	}

	std::string MakeAbilityTriggerCooldownKey(
		const std::string& abilityId,
		const AbilityTrigger& trigger,
		std::size_t triggerIndex
	)
	{
		return abilityId + "|" + trigger.eventTag.ToString() + "|" +
			std::to_string(triggerIndex);
	}
}
