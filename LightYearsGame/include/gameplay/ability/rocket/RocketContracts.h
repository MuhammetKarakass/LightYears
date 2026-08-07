#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::Rocket
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.Rocket.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.Rocket" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.Rocket" };

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] ="Actor.Ability.Rocket.Projectile.Basic";
			inline static const ly::GameplayTag TypeTag{ "AbilityActor.Rocket.Projectile" };
			inline static const ly::GameplayTag AttributeRoot{"Attribute.AbilityActor.Rocket.Projectile"};
			inline static const ly::GameplayTag ProjectileSpeed{"Attribute.AbilityActor.Rocket.Projectile.ProjectileSpeed"};
		};
	};
}
