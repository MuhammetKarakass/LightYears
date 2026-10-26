#pragma once

#include "attributes/AttributeMath.h"

#include <algorithm>

namespace vega
{
	// Ability Haste rating (Owner.AbilityHaste, PercentageRatingScale = 100) divides the leveled cooldown:
	// +100 Haste halves it. Negative Haste counts as zero.
	inline float GetAbilityHasteCooldownMultiplier(float hastePercent) { return 1.f / (1.f + std::max(0.f, hastePercent) / sas::AttributeMath::PercentageRatingScale); }
}
