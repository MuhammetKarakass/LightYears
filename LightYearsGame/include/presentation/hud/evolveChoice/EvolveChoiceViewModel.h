#pragma once

#include "widget/UIViewModel.h"
#include "gameplay/ability/content/AbilityEvolveRules.h"

namespace vega
{
	struct EvolveChoiceViewModel
	{
		bool open = false;
		std::string abilityId;
		std::string abilityName;
		int tierLevel = 0;
		List<AbilityEvolveOption> options;
		UIRevision revision;
	};
}
