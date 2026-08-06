#pragma once

#include "framework/Core.h"

namespace AbilityData::Rocket
{
	inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.Rocket" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.Rocket" };

	struct ActorSchema
	{
		inline static const ly::GameplayTag TypeId{ "AbilityActor.Rocket.Projectile" };
		inline static const ly::GameplayTag AttributeRoot{ "Attribute.AbilityActor.Rocket" };
		inline static const ly::GameplayTag ProjectileSpeed{
			"Attribute.AbilityActor.Rocket.ProjectileSpeed"
		};
	};
}
