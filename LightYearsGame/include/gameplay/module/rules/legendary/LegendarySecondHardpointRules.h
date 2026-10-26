#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendarySecondHardpointRule
	{
		ModuleLevelTable<float> markDamageBonusByLevel;
		float markSeconds = 0.f;
		float switchSeconds = 0.f;
	};

	struct ResolvedModuleLegendarySecondHardpointRule
	{
		std::uint32_t instanceId = 0;
		float markDamageBonus = 0.f;
		float markSeconds = 0.f;
		float switchSeconds = 0.f;
	};
}
