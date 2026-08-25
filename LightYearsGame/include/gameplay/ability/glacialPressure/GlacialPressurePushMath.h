#pragma once

#include <algorithm>

namespace ly::glacialPressure
{
	// Segment falloff and maximum-health scaling are independent balance axes.
	// The former may reduce range for distant targets; the latter can only extend
	// the resolved segment distance rather than being capped back to the base.
	inline float ResolvePushDistance(
		float baseDistance,
		float segmentMultiplier,
		float maximumHealthMultiplier
	)
	{
		return std::max(0.f, baseDistance) *
			std::max(0.f, segmentMultiplier) *
			std::max(1.f, maximumHealthMultiplier);
	}
}
