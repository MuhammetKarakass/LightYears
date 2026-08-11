#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"

namespace AbilityData::Rocket
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.Rocket.Basic";
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.Rocket.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.Rocket.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.Rocket.Projectile.ProjectileSpeed"
			};
		};
	};
}
