#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::CryoBola
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Control.CryoBola.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Control };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.CryoBola" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::CryoBola
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.CryoBola.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.CryoBola.Projectile"
			};
			// Speed and rupture radius belong to the projectile family. Damage,
			// range and collision radius use the existing common attributes.
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.CryoBola.Projectile.ProjectileSpeed"
			};
			inline static const sas::AttributeId RuptureRadius{
				"AbilityActor.CryoBola.Projectile.RuptureRadius"
			};
		};
	};
}
