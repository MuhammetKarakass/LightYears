#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/effects/EffectBehaviorKeys.h"

namespace vega
{
	// Typed fallback used while abilities validate before the JSON effect catalog loads.
	// The source's evolve patch owns the reduction fraction and charge duration.
	inline const sas::GameplayEffectDefinition HullShockReactiveCapacitorEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::HullShock::Effect::ReactiveCapacitor;
		definition.behaviorKey = EffectBehaviorKeys::DamageReduction;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.sourceScopedApplication = true;
		definition.sourceParameterized = true;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.DamageReduction";
		return definition;
	}();
}
