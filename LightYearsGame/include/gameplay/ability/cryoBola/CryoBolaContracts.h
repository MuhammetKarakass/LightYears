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
			// Flight, rupture and damage scaling belong to the projectile family.
			// Base damage, range and collision radius use common attributes.
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.CryoBola.Projectile.ProjectileSpeed"
			};
			inline static const sas::AttributeId RuptureRadius{
				"AbilityActor.CryoBola.Projectile.RuptureRadius"
			};
			inline static const sas::AttributeId EnergyPowerDamageScale{
				"AbilityActor.CryoBola.Projectile.EnergyPowerDamageScale"
			};
			inline static const sas::AttributeId DirectHitDamageMultiplier{
				"AbilityActor.CryoBola.Projectile.DirectHitDamageMultiplier"
			};
		};
	};
}
