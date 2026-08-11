#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"

namespace AbilityData::GravityAnomaly
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Control.GravityAnomaly.Basic";
	};

	struct Effect
	{
		inline static constexpr char InsideEffectId[] =
			"Effect.GravityAnomaly.Inside.Basic";
		// C++ fallback/test value. Shipped content owns this in abilities.json.
		inline static constexpr float InsideEffectDurationSeconds = 2.f;
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.GravityAnomaly.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.GravityAnomaly.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.GravityAnomaly.Projectile.ProjectileSpeed"
			};
		};

		struct Field
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.GravityAnomaly.Field.Basic";
			inline static constexpr char BasicAttributeProfileId[] =
				"AttributeProfile.GravityAnomaly.Field.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.GravityAnomaly.Field"
			};
			inline static const sas::AttributeId PullStrength{
				"AbilityActor.GravityAnomaly.Field.PullStrength"
			};
			inline static const sas::AttributeId SlowMagnitude{
				"AbilityActor.GravityAnomaly.Field.SlowMagnitude"
			};
			inline static const sas::AttributeId InsideEffectDuration{
				"AbilityActor.GravityAnomaly.Field.InsideEffectDuration"
			};
		};
	};
}
