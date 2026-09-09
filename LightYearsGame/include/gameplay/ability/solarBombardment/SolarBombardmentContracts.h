#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::SolarBombardment
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.SolarBombardment.Basic";
	};

	inline const ly::GameplayTag CategoryTag{
		ly::GameplayTagSchema::AbilityOffense
	};
	inline const ly::GameplayTag BehaviorTag{
		"GameAbilityBehavior.SolarBombardment"
	};
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::SolarBombardment
	};

	struct Attribute
	{
		// Common damage/range/radius channels stay shared with other abilities.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId OuterRadius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId CastRange = ly::CommonAttributeIds::Range;

		// These values are genuinely owned by this projectile family because they
		// describe its two-zone delivery and distance-based travel curve.
		inline static const sas::AttributeId InnerRadius{
			"AbilityActor.SolarBombardment.Projectile.InnerRadius"
		};
		inline static const sas::AttributeId InnerDamageMultiplier{
			"AbilityActor.SolarBombardment.Projectile.InnerDamageMultiplier"
		};
		inline static const sas::AttributeId InnerIgniteStacks{
			"AbilityActor.SolarBombardment.Projectile.InnerIgniteStacks"
		};
		inline static const sas::AttributeId OuterIgniteStacks{
			"AbilityActor.SolarBombardment.Projectile.OuterIgniteStacks"
		};
		inline static const sas::AttributeId MinTravelTime{
			"AbilityActor.SolarBombardment.Projectile.MinTravelTime"
		};
		inline static const sas::AttributeId MaxTravelTime{
			"AbilityActor.SolarBombardment.Projectile.MaxTravelTime"
		};
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.SolarBombardment.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.SolarBombardment.Projectile"
			};
		};
	};
}
