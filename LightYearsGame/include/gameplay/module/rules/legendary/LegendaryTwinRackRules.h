#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryTwinRackRule
	{
		ModuleLevelTable<float> pairedRefundFractionByLevel;
		float cooldownMultiplier = 0.f;
		int abilitiesPerSlot = 0;
	};

	struct ResolvedModuleLegendaryTwinRackRule
	{
		std::uint32_t instanceId = 0;
		float pairedRefundFraction = 0.f;
		float cooldownMultiplier = 0.f;
		int abilitiesPerSlot = 0;
	};
}
