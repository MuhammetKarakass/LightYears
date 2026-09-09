#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::AegisReaver
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.AegisReaver.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.AegisReaver" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::AegisReaver;

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.AegisReaver.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.AegisReaver.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.AegisReaver.Projectile.ProjectileSpeed"
			};
			inline static const sas::AttributeId ReturnSpeed{
				"AbilityActor.AegisReaver.Projectile.ReturnSpeed"
			};
			inline static const sas::AttributeId ShieldConversionRatio{
				"AbilityActor.AegisReaver.Projectile.ShieldConversionRatio"
			};
			inline static const sas::AttributeId ShieldStealRatio{
				"AbilityActor.AegisReaver.Projectile.ShieldStealRatio"
			};
		};
	};
}
