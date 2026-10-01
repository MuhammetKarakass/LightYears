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
}
