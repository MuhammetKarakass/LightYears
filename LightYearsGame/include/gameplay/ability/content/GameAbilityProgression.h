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

	inline float GetGlobalAbilityCooldownStepReduction(
		float baseCooldown,
		std::size_t zeroBasedUpgradeIndex
	)
	{
		if (baseCooldown <= 1.f)
		{
			return 0.f;
		}

		float remainingCooldown = baseCooldown;
		float reductionPerLevel = 0.175f + 0.025f * baseCooldown;
		for (std::size_t index = 0; index <= zeroBasedUpgradeIndex; ++index)
		{
			if (index > 0 && index % 4 == 0)
			{
				reductionPerLevel = reductionPerLevel >= 0.20f
					? reductionPerLevel - 0.10f
					: reductionPerLevel * 0.80f;
			}

			const float reduction = std::min(reductionPerLevel, remainingCooldown - 1.f);
			if (index == zeroBasedUpgradeIndex)
			{
				return std::max(0.f, reduction);
			}
			remainingCooldown -= reduction;
		}
		return 0.f;
	}

	inline List<sas::AttributeModifier> MakeGlobalAbilityCooldownProgression(
		float baseCooldown,
		std::size_t stepCount
	)
	{
		List<sas::AttributeModifier> modifiers;
		modifiers.reserve(stepCount);
		for (std::size_t index = 0; index < stepCount; ++index)
		{
			modifiers.emplace_back(
				CommonAttributeIds::Cooldown,
				-GetGlobalAbilityCooldownStepReduction(baseCooldown, index)
			);
		}
		return modifiers;
	}
}
