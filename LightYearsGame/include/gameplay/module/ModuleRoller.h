#pragma once

#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ModuleSideFeature.h"

#include <cstdint>
#include <optional>

namespace vega
{
	namespace ModuleRoller
	{
		// Advances only the caller-owned state; sampling is explicit and platform-stable.
		std::optional<sas::ContentId> RollSideFeatureId(const ModuleDefinition& module, const List<ModuleSideFeaturePool>& pools, const ModuleSideFeatureLookup& lookup, std::uint64_t& seed);
	}
}
