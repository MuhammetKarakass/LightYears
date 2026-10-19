#pragma once

#include "framework/MathUtility.h"

// Forced into build-tree copies of the four stage sources only. Production code
// still calls Vega's RandRange, with its original random_device initialization.
namespace vega
{
	inline std::mt19937 campaignTestRandom{ 1741 };
	template<typename T> T CampaignTestRandRange(T minimum, T maximum)
	{
		if constexpr (std::is_integral_v<T>) return std::uniform_int_distribution<T>{ minimum, maximum }(campaignTestRandom);
		else return std::uniform_real_distribution<T>{ minimum, maximum }(campaignTestRandom);
	}
}
#define RandRange CampaignTestRandRange
