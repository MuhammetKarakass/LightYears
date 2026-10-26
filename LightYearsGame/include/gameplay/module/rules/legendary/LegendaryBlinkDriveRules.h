#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryBlinkDriveRule
	{
		ModuleLevelTable<float> rechargeSecondsByLevel;
		ModuleLevelTable<float> movementSpeedBonusByLevel;
		int charges = 0;
		float useCooldownSeconds = 0.f;
		float boostSeconds = 0.f;
		float distance = 0.f;
	};

	struct ResolvedModuleLegendaryBlinkDriveRule
	{
		std::uint32_t instanceId = 0;
		float rechargeSeconds = 0.f;
		float movementSpeedBonus = 0.f;
		int charges = 0;
		float useCooldownSeconds = 0.f;
		float boostSeconds = 0.f;
		float distance = 0.f;
	};
}
