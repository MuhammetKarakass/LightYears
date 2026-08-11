#pragma once

#include "effects/GameplayEffectBehaviorKey.h"

namespace ly::EffectBehaviorKeys
{
	// These keys select closed implementation hooks at the JSON/registry
	// boundary. They are not gameplay tags and do not describe game semantics.
	inline const sas::GameplayEffectBehaviorKey Barrier{
		"EffectBehavior.Barrier"
	};
	inline const sas::GameplayEffectBehaviorKey DamageIgnite{
		"EffectBehavior.Damage.Ignite"
	};
	inline const sas::GameplayEffectBehaviorKey DamageElectric{
		"EffectBehavior.Damage.Electric"
	};
	inline const sas::GameplayEffectBehaviorKey GravityAnomaly{
		"EffectBehavior.GravityAnomaly"
	};
}
