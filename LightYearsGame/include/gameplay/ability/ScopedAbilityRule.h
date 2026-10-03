#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"

namespace ly
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
		float multiplier = 1.f;
	};
}
