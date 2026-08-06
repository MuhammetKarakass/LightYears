#pragma once

#include "framework/Core.h"

namespace AbilityData::GravityAnomaly
{
	inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.GravityAnomaly" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Control.GravityAnomaly" };

	struct EffectSchema
	{
		inline static const ly::GameplayTag BehaviorId{
			"EffectBehavior.GravityAnomaly"
		};
		inline static constexpr char InsideEffectId[] =
			"Effect.GravityAnomaly.Inside";
		inline static const ly::GameplayTag InsideTag{
			"Effect.GravityAnomaly.Inside"
		};
		// C++ fallback/test value. Shipped content owns this in abilities.json.
		inline static constexpr float InsideEffectDurationSeconds = 2.f;
	};

	struct ActorSchema
	{
		inline static const ly::GameplayTag AttributeRoot{
			"Attribute.AbilityActor.GravityAnomaly"
		};
		inline static const ly::GameplayTag ProjectileTypeId{
			"AbilityActor.GravityAnomaly.Projectile"
		};
		inline static const ly::GameplayTag FieldTypeId{
			"AbilityActor.GravityAnomaly.Field"
		};
		inline static const ly::GameplayTag ProjectileSpeed{
			"Attribute.AbilityActor.GravityAnomaly.ProjectileSpeed"
		};
		inline static const ly::GameplayTag CastRange{
			"Attribute.AbilityActor.GravityAnomaly.CastRange"
		};
		inline static const ly::GameplayTag PullStrength{
			"Attribute.AbilityActor.GravityAnomaly.PullStrength"
		};
		inline static const ly::GameplayTag SlowMagnitude{
			"Attribute.AbilityActor.GravityAnomaly.SlowMagnitude"
		};
		inline static const ly::GameplayTag InsideEffectDuration{
			"Attribute.AbilityActor.GravityAnomaly.InsideEffectDuration"
		};
	};
}
