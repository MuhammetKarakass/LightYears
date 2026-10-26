#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryUnderdogCoreRule
	{
		ModuleLevelTable<float> offensePowerPerWeightByLevel;
		ModuleLevelTable<float> defenseHealthPerWeightByLevel;
		ModuleLevelTable<float> primaryAttackSpeedPercentPerWeightByLevel;
		ModuleLevelTable<float> movementRatingPerWeightByLevel;
		ModuleLevelTable<float> utilityHastePercentPerWeightByLevel;
		int commonWeight = 0;
		int uncommonWeight = 0;
		int rareWeight = 0;
		int epicWeight = 0;
		int legendaryWeight = 0;
	};

	struct ResolvedModuleLegendaryUnderdogCoreRule
	{
		std::uint32_t instanceId = 0;
		float offensePowerPerWeight = 0.f;
		float defenseHealthPerWeight = 0.f;
		float primaryAttackSpeedPercentPerWeight = 0.f;
		float movementRatingPerWeight = 0.f;
		float utilityHastePercentPerWeight = 0.f;
		int offenseWeight = 0;
		int defenseWeight = 0;
		int primaryWeight = 0;
		int movementWeight = 0;
		int utilityWeight = 0;
		int commonWeight = 0;
		int uncommonWeight = 0;
		int rareWeight = 0;
		int epicWeight = 0;
		int legendaryWeight = 0;
	};
}
