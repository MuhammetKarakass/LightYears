#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryMeltdownBarrelRule
	{
		ModuleLevelTable<float> novaBudgetFractionByLevel;
		float focusSeconds = 0.f;
		float novaRadius = 0.f;
		int igniteStacks = 0;
		float heatCapacity = 0.f;
	};

	struct ResolvedModuleLegendaryMeltdownBarrelRule
	{
		std::uint32_t instanceId = 0;
		float novaBudgetFraction = 0.f;
		float focusSeconds = 0.f;
		float novaRadius = 0.f;
		int igniteStacks = 0;
		float heatCapacity = 0.f;
	};
}
