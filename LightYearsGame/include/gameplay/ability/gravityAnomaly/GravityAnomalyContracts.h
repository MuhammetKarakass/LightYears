#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::GravityAnomaly
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Control.GravityAnomaly.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityControl };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.GravityAnomaly" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Control.GravityAnomaly" };

	struct Effect
	{
		inline static const ly::GameplayTag BehaviorTag{ 
			"EffectBehavior.GravityAnomaly"
		};
		inline static constexpr char InsideEffectId[] =
			"Effect.GravityAnomaly.Inside.Basic";
		inline static const ly::GameplayTag InsideTag{
			"State.Effect.GravityAnomaly.Inside"
		};
		// C++ fallback/test value. Shipped content owns this in abilities.json.
		inline static constexpr float InsideEffectDurationSeconds = 2.f;
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.GravityAnomaly.Projectile.Basic";
			inline static const ly::GameplayTag TypeTag{
				"AbilityActor.GravityAnomaly.Projectile"
			};
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.GravityAnomaly.Projectile"
			};
			inline static const ly::GameplayTag ProjectileSpeed{
				"Attribute.AbilityActor.GravityAnomaly.Projectile.ProjectileSpeed"
			};
		};

		struct Field
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.GravityAnomaly.Field.Basic";
			inline static constexpr char BasicAttributeProfileId[] =
				"AttributeProfile.GravityAnomaly.Field.Basic";
			inline static const ly::GameplayTag TypeTag{ "AbilityActor.GravityAnomaly.Field" };
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.GravityAnomaly.Field"
			};
			inline static const ly::GameplayTag PullStrength{
				"Attribute.AbilityActor.GravityAnomaly.Field.PullStrength"
			};
			inline static const ly::GameplayTag SlowMagnitude{
				"Attribute.AbilityActor.GravityAnomaly.Field.SlowMagnitude"
			};
			inline static const ly::GameplayTag InsideEffectDuration{
				"Attribute.AbilityActor.GravityAnomaly.Field.InsideEffectDuration"
			};
		};
	};
}
