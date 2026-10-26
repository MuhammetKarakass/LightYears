#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryParasiteHullRule
	{
		ModuleLevelTable<float> borrowedHealthBonusByLevel;
		ModuleLevelTable<float> borrowedDamageBonusByLevel;
		float maximumSeconds = 0.f;
		float repairMaxHealthFractionPerSecond = 0.f;
		float minimumReturnHealthFraction = 0.f;
		float cooldownSeconds = 0.f;
		float possessionRange = 0.f;
	};

	struct ResolvedModuleLegendaryParasiteHullRule
	{
		std::uint32_t instanceId = 0;
		float borrowedHealthBonus = 0.f;
		float borrowedDamageBonus = 0.f;
		float maximumSeconds = 0.f;
		float repairMaxHealthFractionPerSecond = 0.f;
		float minimumReturnHealthFraction = 0.f;
		float cooldownSeconds = 0.f;
		float possessionRange = 0.f;
	};
}
