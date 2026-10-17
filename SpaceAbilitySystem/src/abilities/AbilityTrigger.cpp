#include "abilities/AbilityTrigger.h"

namespace sas
{
	bool MatchesAbilityTrigger(
		const AbilityEvent& event,
		const AbilityTrigger& trigger,
		const vega::GameplayTagContainer& ownedTags
	)
	{
		vega::GameplayTagContainer payloadTags;
		for (const vega::GameplayTag& tag : event.payloadTags)
		{
			payloadTags.AddTag(tag);
		}
		vega::GameplayTagContainer sourceAbilityTags;
		for (const vega::GameplayTag& tag : event.sourceAbilityTags)
		{
			sourceAbilityTags.AddTag(tag);
		}

		return event.eventTag.MatchesTag(trigger.eventTag) &&
			(!trigger.abilityId.IsValid() || event.sourceAbilityId == trigger.abilityId) &&
			!trigger.endReason.has_value() &&
			sourceAbilityTags.HasAll(trigger.requiredAbilityTags) &&
			!sourceAbilityTags.HasAny(trigger.blockedAbilityTags) &&
			payloadTags.HasAll(trigger.requiredPayloadTags) &&
			!payloadTags.HasAny(trigger.blockedPayloadTags) &&
			ownedTags.HasAll(trigger.requiredTags) &&
			!ownedTags.HasAny(trigger.blockedTags);
	}

	bool MatchesAbilityTrigger(
		const AbilityLifecycleEvent& event,
		const AbilityTrigger& trigger,
		const vega::GameplayTagContainer& ownedTags
	)
	{
		vega::GameplayTagContainer payloadTags;
		for (const vega::GameplayTag& tag : event.payloadTags)
		{
			payloadTags.AddTag(tag);
		}
		vega::GameplayTagContainer sourceAbilityTags;
		for (const vega::GameplayTag& tag : event.abilityTags)
		{
			sourceAbilityTags.AddTag(tag);
		}

		return event.eventTag.MatchesTag(trigger.eventTag) &&
			(!trigger.abilityId.IsValid() || event.abilityId == trigger.abilityId) &&
			(!trigger.endReason.has_value() || event.endReason == *trigger.endReason) &&
			sourceAbilityTags.HasAll(trigger.requiredAbilityTags) &&
			!sourceAbilityTags.HasAny(trigger.blockedAbilityTags) &&
			payloadTags.HasAll(trigger.requiredPayloadTags) &&
			!payloadTags.HasAny(trigger.blockedPayloadTags) &&
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
