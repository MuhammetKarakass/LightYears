#pragma once

#include "framework/Core.h"
#include <string>

namespace vega
{
	struct FieldMasteryRunState
	{
		struct UseCounter { std::string abilityId; double uses = 0.0; };
		List<UseCounter> counters;
	};
}
