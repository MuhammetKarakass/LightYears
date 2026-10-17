#pragma once

#include "abilities/AbilityTypes.h"
#include "attributes/AttributeSystem.h"
#include "framework/Core.h"

namespace vega
{
	struct ScopedAbilityRule
	{
		List<GameplayTag> requiredAbilityTags;
		List<GameplayTag> blockedAbilityTags;
		List<sas::AttributeModifier> attributeModifiers;
		int levelBonus = 0;
	};

	struct ScopedAbilityCooldownRule
	{
		List<GameplayTag> requiredAbilityTags;
		// None matches every slot; a loadout slot restricts the rule to that one ability (Bounty Mark neighbours).
		sas::AbilitySlot requiredSlot = sas::AbilitySlot::None;
		float multiplier = 1.f;
		// Absolute floor on the resolved cooldown in seconds, applied only while the rule reduces the cooldown. 0 = none.
		float minimumSeconds = 0.f;
	};
}
