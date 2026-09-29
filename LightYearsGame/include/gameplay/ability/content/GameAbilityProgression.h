#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>
#include <cstddef>

namespace ly
{
	struct AbilityLevelStep
	{
		List<sas::AttributeModifier> attributeModifiers;
		List<std::string> unlockedUpgradeIds;
		List<AbilityActionSpec> addedActions;
		List<AbilityTriggerSpec> addedTriggers;
		List<sas::AttributeScalingRule> scalingRules;
	};

	inline List<AbilityLevelStep> MakeRepeatedAbilityLevelProgression(
		std::size_t stepCount,
		const AbilityLevelStep& step
	)
	{
		return List<AbilityLevelStep>(stepCount, step);
	}

	// Global cooldown progression. Step index 0 is the upgrade that reaches level two.
	inline float GetGlobalAbilityCooldownStepReduction(
		float baseCooldown,
		std::size_t zeroBasedUpgradeIndex
	)
	{
		if (baseCooldown <= 1.f)
		{
			return 0.f;
		}

		float reductionPerLevel = 0.175f + 0.025f * baseCooldown;
		for (std::size_t index = 1; index <= zeroBasedUpgradeIndex; ++index)
		{
			if (index % 4 == 0)
			{
				reductionPerLevel = reductionPerLevel >= 0.20f
					? reductionPerLevel - 0.10f
					: reductionPerLevel * 0.80f;
			}
		}
		return std::max(reductionPerLevel, 0.02f);
	}

	// Cumulative reduction after stepsGained upgrades, clamped so the cooldown stays >= 1s.
	inline float GetGlobalAbilityCooldownTotalReduction(
		float baseCooldown,
		std::size_t stepsGained
	)
	{
		if (baseCooldown <= 1.f)
		{
			return 0.f;
		}

		const float maxReduction = baseCooldown - 1.f;
		float total = 0.f;
		float reductionPerLevel = 0.175f + 0.025f * baseCooldown;
		for (std::size_t index = 0; index < stepsGained; ++index)
		{
			if (index > 0 && index % 4 == 0)
			{
				reductionPerLevel = reductionPerLevel >= 0.20f
					? reductionPerLevel - 0.10f
					: reductionPerLevel * 0.80f;
			}
			total += std::max(reductionPerLevel, 0.02f);
			if (total >= maxReduction)
			{
				return maxReduction;
			}
		}
		return total;
	}

	// Templated to avoid a circular include with GameAbilityDefinition.h.
	template <typename DefinitionT>
	inline void SetRepeatingAbilityLevelStep(
		DefinitionT& definition,
		const AbilityLevelStep& step
	)
	{
		definition.repeatingLevelProgression.clear();
		definition.repeatingLevelProgression.push_back(step);
	}
}
