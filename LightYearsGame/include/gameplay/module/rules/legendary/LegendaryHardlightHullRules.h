#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryHardlightHullRule
	{
		ModuleLevelTable<float> healthToShieldFractionByLevel;
		float maxHealth = 0.f;
	};

	struct ResolvedModuleLegendaryHardlightHullRule
	{
		std::uint32_t instanceId = 0;
		float healthToShieldFraction = 0.f;
		float maxHealth = 0.f;
	};
}
