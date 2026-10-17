#pragma once

#include "gameplay/targeting/TargetingTypes.h"
#include "framework/Core.h"

#include <cstddef>

namespace vega::targeting
{
	class TargetAllocation
	{
	public:
		static List<weak_ptr<Actor>> DistributeEvenly(const List<TargetingCandidate>& candidates, std::size_t projectileCount);
	};
}