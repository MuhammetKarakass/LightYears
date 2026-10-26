#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryPrimedArsenalRule
	{
		ModuleLevelTable<float> powerPerReadySecondByLevel;
		float readyWindowSeconds = 0.f;
	};

	struct ResolvedModuleLegendaryPrimedArsenalRule
	{
		std::uint32_t instanceId = 0;
		float powerPerReadySecond = 0.f;
		float readyWindowSeconds = 0.f;
	};
}
