#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::CrescentReaver
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.CrescentReaver.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.CrescentReaver" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::CrescentReaver;

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.CrescentReaver.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.CrescentReaver.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.CrescentReaver.Projectile.ProjectileSpeed"
			};
			inline static const sas::AttributeId BounceCount{
				"AbilityActor.CrescentReaver.Projectile.BounceCount"
			};
			inline static const sas::AttributeId BounceDamageGrowth{
				"AbilityActor.CrescentReaver.Projectile.BounceDamageGrowth"
			};
			inline static const sas::AttributeId BounceCooldownReduction{
				"AbilityActor.CrescentReaver.Projectile.BounceCooldownReduction"
			};
		};
	};
}
