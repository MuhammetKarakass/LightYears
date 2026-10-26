#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryNeuralHijackerRule
	{
		ModuleLevelTable<float> allySecondsByLevel;
		float allyHealthFraction = 0.f;
		float allyDamageMultiplier = 0.f;
	};

	struct ResolvedModuleLegendaryNeuralHijackerRule
	{
		std::uint32_t instanceId = 0;
		float allySeconds = 0.f;
		float allyHealthFraction = 0.f;
		float allyDamageMultiplier = 0.f;
	};
}
