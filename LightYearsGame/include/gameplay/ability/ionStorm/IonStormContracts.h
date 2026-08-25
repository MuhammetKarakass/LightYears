#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::IonStorm
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.IonStorm.Basic";
	};

	inline const ly::GameplayTag CategoryTag{
		ly::GameplayTagSchema::AbilityOffense
	};
	inline const ly::GameplayTag BehaviorTag{
		"GameAbilityBehavior.IonStorm"
	};
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::IonStorm
	};

	struct Attribute
	{
		// Shared channels are deliberately reused by the generic resolver.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId Duration = ly::CommonAttributeIds::Duration;
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId CastRange = ly::CommonAttributeIds::Range;

		// These values describe Ion Storm's delivery and irregular boundary.
		inline static const sas::AttributeId ProjectileSpeed{
			"AbilityActor.IonStorm.Projectile.ProjectileSpeed"
		};
		inline static const sas::AttributeId TickInterval{
			"AbilityActor.IonStorm.Field.TickInterval"
		};
		inline static const sas::AttributeId InnerCoreRadius{
			"AbilityActor.IonStorm.Field.InnerCoreRadius"
		};
		inline static const sas::AttributeId OuterMinRadius{
			"AbilityActor.IonStorm.Field.OuterMinRadius"
		};
		inline static const sas::AttributeId OuterMaxRadius{
			"AbilityActor.IonStorm.Field.OuterMaxRadius"
		};
		inline static const sas::AttributeId BoundaryPointCount{
			"AbilityActor.IonStorm.Field.BoundaryPointCount"
		};
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.IonStorm.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.IonStorm.Projectile"
			};
		};

		struct Field
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.IonStorm.Field.Basic";
			inline static constexpr char BasicAttributeProfileId[] =
				"AttributeProfile.IonStorm.Field.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.IonStorm.Field"
			};
		};
	};
}
