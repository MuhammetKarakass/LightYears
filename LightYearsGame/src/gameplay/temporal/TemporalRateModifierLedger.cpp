#include "gameplay/temporal/TemporalRateModifierLedger.h"

#include <algorithm>
#include <cmath>

namespace ly::temporal
{
	void TemporalRateModifierLedger::SetModifier(
		RateModifierSourceId sourceId,
		float multiplier
	)
	{
		if (sourceId == 0 || !std::isfinite(multiplier) || multiplier < 0.f)
		{
			return;
		}
		mModifiers[sourceId] = multiplier;
	}

	void TemporalRateModifierLedger::RemoveModifier(RateModifierSourceId sourceId)
	{
		mModifiers.erase(sourceId);
	}

	float TemporalRateModifierLedger::ResolveMultiplier() const
	{
		float multiplier = 1.f;
		for (const auto& [sourceId, value] : mModifiers)
		{
			(void)sourceId;
			if (std::isfinite(value))
			{
				multiplier = std::min(multiplier, std::max(0.f, value));
			}
		}
		return multiplier;
	}

	void TemporalRateModifierLedger::Clear()
	{
		mModifiers.clear();
	}
}
