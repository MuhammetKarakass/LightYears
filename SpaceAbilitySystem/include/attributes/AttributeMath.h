#pragma once

#include <algorithm>
#include <cmath>

namespace sas::AttributeMath
{
	// Rating curves are asymptotic: every point contributes, but no percentage
	// stat can ever reach 100%. The scales are centralized for balance tuning.
	inline constexpr float PercentageRatingScale = 100.f;
	inline constexpr float ArmorRatingScale = 50.f / 0.69314718056f;

	inline float SaturatingFraction(float rating, float scale)
	{
		if (rating <= 0.f || scale <= 0.f)
		{
			return 0.f;
		}
		return std::clamp(1.f - std::exp(-rating / scale), 0.f, 1.f);
	}

	inline float GetAbilityHasteReduction(float hasteRating)
	{
		return SaturatingFraction(hasteRating, PercentageRatingScale);
	}

	inline float GetAbilityCooldownMultiplier(float hasteRating)
	{
		return 1.f - GetAbilityHasteReduction(hasteRating);
	}

	inline float GetCriticalChance(float criticalRating, float baseCriticalChance = 0.f)
	{
		const float baseChance = std::clamp(baseCriticalChance, 0.f, 1.f);
		return 1.f - (1.f - baseChance) *
			(1.f - SaturatingFraction(criticalRating, PercentageRatingScale));
	}

	inline float GetCombatLuckFactor(float luckRating)
	{
		return SaturatingFraction(luckRating, PercentageRatingScale);
	}

	inline float GetLootLuckFactor(float luckRating)
	{
		return SaturatingFraction(luckRating, PercentageRatingScale * 3.f);
	}

	inline float ApplyLuckToProcChance(
		float baseProcChance,
		float luckRating,
		float luckScaling = 1.f
	)
	{
		const float baseChance = std::clamp(baseProcChance, 0.f, 1.f);
		const float contribution = std::clamp(
			GetCombatLuckFactor(luckRating) * std::max(0.f, luckScaling),
			0.f,
			1.f
		);
		return baseChance + (1.f - baseChance) * contribution;
	}

	inline float GetArmorDamageReduction(float armorRating)
	{
		return SaturatingFraction(armorRating, ArmorRatingScale);
	}
}
