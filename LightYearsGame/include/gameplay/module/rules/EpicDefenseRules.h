#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstddef>

namespace vega
{
	template<typename T>
	T ResolveEpicLevelValue(const ModuleLevelTable<T>& table, T fallback, int level) { return table.ValueAtLevel(fallback, level); }

	// Last Stand (main package). Lethal damage revives the ship while charges remain: health is set to healthFractionByLevel of
	// MaxHealth. The combat stats are multiplied by bonusMultiplier for bonusSeconds after each revive.
	struct ModuleReviveRule
	{
		ModuleLevelTable<float> healthFractionByLevel;
		ModuleLevelTable<int> chargesByLevel;
		float bonusSeconds = 0.f;
		float bonusMultiplier = 1.f;
	};

	// Emergency Protocol. While the owner's health fraction is at most healthAtMost, the effects apply: cooldownRecovery (main
	// package, per level) speeds Q/E/F/R cooldown recovery, abilityDamage scales outgoing Q/E/F/R damage and movementSpeed scales
	// movement speed (side features). Each unset multiplier is neutral.
	struct ModuleHealthThresholdRule
	{
		float healthAtMost = 0.f;
		ModuleLevelTable<float> cooldownRecoveryMultiplierByLevel;
		float abilityDamageMultiplier = 1.f;
		float movementSpeedMultiplier = 1.f;
	};
}
