#pragma once

#include <algorithm>

namespace ly::time
{
	constexpr int DefaultMaximumIntervalCatchUp = 4;
	constexpr float MinimumIntervalSeconds = 0.001f;

	inline void AdvanceIntervalDebt(float& remainingSeconds, float deltaTime)
	{
		remainingSeconds -= std::max(0.f, deltaTime);
	}

	inline void CommitInterval(float& remainingSeconds, float intervalSeconds)
	{
		remainingSeconds += std::max(MinimumIntervalSeconds, intervalSeconds);
	}
}
