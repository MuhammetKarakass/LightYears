#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/content/GameAbilityActions.h"

#include <cstddef>

namespace ly
{
	struct AbilityLevelStep
	{
		List<sas::AttributeModifier> attributeModifiers;
		List<GameplayTag> unlockedUpgradeIds;
		List<AbilityActionSpec> addedActions;
		List<AbilityTriggerSpec> addedTriggers;
	};

	inline List<AbilityLevelStep> MakeRepeatedAbilityLevelProgression(
		std::size_t stepCount,
		const AbilityLevelStep& step
	)
	{
		return List<AbilityLevelStep>(stepCount, step);
	}
}
