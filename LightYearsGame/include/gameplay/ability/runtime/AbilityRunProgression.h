#pragma once

#include "framework/Core.h"
#include <string>

namespace vega
{
	// Player-owned run progression. The ASC holds a weak view for subsequent grants.
	struct AbilityRunProgression
	{
		Map<std::string, int> purchasedLevels;
		Map<std::string, List<std::string>> evolveSelections;
	};
}
