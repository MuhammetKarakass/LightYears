#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::RailBurst
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.RailBurst.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.RailBurst" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::RailBurst;

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.RailBurst.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.RailBurst.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.RailBurst.Projectile.ProjectileSpeed"
			};
		};
	};
}
