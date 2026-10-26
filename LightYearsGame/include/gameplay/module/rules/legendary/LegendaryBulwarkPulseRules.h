#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryBulwarkPulseRule
	{
		ModuleLevelTable<float> baseDamageByLevel;
		ModuleLevelTable<float> healthCoefficientByLevel;
		float healthThreshold = 0.f;
		float radius = 0.f;
		float cooldownSeconds = 0.f;
	};

	struct ResolvedModuleLegendaryBulwarkPulseRule
	{
		std::uint32_t instanceId = 0;
		float baseDamage = 0.f;
		float healthCoefficient = 0.f;
		float healthThreshold = 0.f;
		float radius = 0.f;
		float cooldownSeconds = 0.f;
	};
}
