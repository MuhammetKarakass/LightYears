#pragma once

#include <initializer_list>

namespace vega
{
	// Only main-effect module power belongs here; owner stats, crit and target scaling remain separate.
	inline float ResolveMainEffectMultiplier(std::initializer_list<float> multipliers)
	{
		float result = 1.f;
		for (float multiplier : multipliers) result += multiplier - 1.f;
		return result;
	}
}
