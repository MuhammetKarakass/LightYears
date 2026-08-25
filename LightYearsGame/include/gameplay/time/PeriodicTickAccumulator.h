#pragma once

#include <algorithm>
#include <cmath>

namespace ly::time
{
	inline constexpr int DefaultMaximumCatchUpTicks = 4;

	// Preserves elapsed time while placing a fixed ceiling on the work performed
	// in one frame. Remaining debt is processed by later frames instead of
	// turning a hitch into an unbounded burst of damage/effect callbacks.
	inline int ConsumePeriodicTicks(
		float& accumulator,
		float deltaTime,
		float interval,
		int maximumTicks = DefaultMaximumCatchUpTicks
	)
	{
		const float safeInterval = std::max(0.001f, interval);
		accumulator = std::max(0.f, accumulator) + std::max(0.f, deltaTime);
		const int dueTicks = static_cast<int>(std::floor(accumulator / safeInterval));
		const int consumedTicks = std::min(
			std::max(0, dueTicks),
			std::max(0, maximumTicks)
		);
		accumulator -= safeInterval * static_cast<float>(consumedTicks);
		return consumedTicks;
	}
}
