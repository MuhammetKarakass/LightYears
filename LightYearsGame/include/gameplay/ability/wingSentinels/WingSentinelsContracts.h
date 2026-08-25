#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::WingSentinels
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.WingSentinels.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.WingSentinels" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::WingSentinels;

	struct State
	{
		inline static const ly::GameplayTag Active = ly::GameplayTags::State::Ability::WingSentinels::Active;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started = ly::GameplayTags::Event::Ability::WingSentinels::Started;
		inline static const ly::GameplayTag Ended = ly::GameplayTags::Event::Ability::WingSentinels::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId DroneCount{ "Ability.Offense.WingSentinels.DroneCount" };
		inline static const sas::AttributeId SideOffset{ "Ability.Offense.WingSentinels.SideOffset" };
		inline static const sas::AttributeId BaseAttackRate{ "Ability.Offense.WingSentinels.BaseAttackRate" };
		// Both targeting and projectile range are intentionally shared Range;
		// they have the same balance meaning in this ability family.
		inline static const sas::AttributeId Range = ly::CommonAttributeIds::Range;
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.WingSentinels.Projectile.Basic";
			inline static const sas::AttributeId Root{ "AbilityActor.WingSentinels.Projectile" };
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.WingSentinels.Projectile.ProjectileSpeed"
			};
		};
	};
}
