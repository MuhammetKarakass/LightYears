#pragma once

#include "framework/Core.h"

#include <cstdint>

namespace ly::temporal
{
	using RateModifierSourceId = std::uint64_t;

	// A small source-keyed ledger for gameplay rates that must be slowed without
	// changing the owner's canonical attributes. The lowest active multiplier
	// wins, so independent temporal effects cannot accidentally multiply into an
	// uncontrolled value and each source can clean up independently.
	class TemporalRateModifierLedger final
	{
	public:
		void SetModifier(RateModifierSourceId sourceId, float multiplier);
		void RemoveModifier(RateModifierSourceId sourceId);
		float ResolveMultiplier() const;
		void Clear();

	private:
		Map<RateModifierSourceId, float> mModifiers;
	};
}
