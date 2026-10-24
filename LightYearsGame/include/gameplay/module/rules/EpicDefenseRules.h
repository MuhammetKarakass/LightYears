#pragma once

#include "framework/Core.h"

#include <cstddef>

namespace vega
{
	// Per-level value of an Epic Defense rule. Entries start at level 1; levels past the last entry extrapolate linearly from the
	// last step, as ModuleLevelTable does. An empty list yields the fallback.
	template<typename T>
	T ResolveEpicLevelValue(const List<T>& values, T fallback, int level)
	{
		if (values.empty() || level <= 0) return fallback;
		const int count = static_cast<int>(values.size());
		if (level <= count) return values[static_cast<std::size_t>(level - 1)];
		if (count == 1) return values.front();
		const T step = values.back() - values[static_cast<std::size_t>(count - 2)];
		return values.back() + step * static_cast<T>(level - count);
	}

	// Last Stand (main package). Lethal damage revives the ship while charges remain: health is set to healthFractionByLevel of
	// MaxHealth. The combat stats are multiplied by bonusMultiplier for bonusSeconds after each revive.
	struct ModuleReviveRule
	{
		List<float> healthFractionByLevel;
		List<int> chargesByLevel;
		float bonusSeconds = 0.f;
		float bonusMultiplier = 1.f;
	};

	// Emergency Protocol. While the owner's health fraction is at most healthAtMost, the effects apply: cooldownRecovery (main
	// package, per level) speeds Q/E/F/R cooldown recovery, abilityDamage scales outgoing Q/E/F/R damage and movementSpeed scales
	// movement speed (side features). Each unset multiplier is neutral.
	struct ModuleHealthThresholdRule
	{
		float healthAtMost = 0.f;
		List<float> cooldownRecoveryMultiplierByLevel;
		float abilityDamageMultiplier = 1.f;
		float movementSpeedMultiplier = 1.f;
	};
}
