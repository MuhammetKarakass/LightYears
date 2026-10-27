#pragma once
#include "gameplay/damage/DamageContext.h"

namespace vega
{
	// Converts a hit before any outgoing/incoming type reader sees it. Stack count is carried across types;
	// hits with no damage status gain none. Status DoT and generated module hits keep their own identity.
	void ApplyDamageTypeOverride(List<GameplayTag>& damageTags, DamagePayload& payload, const GameplayTag& type);
}
