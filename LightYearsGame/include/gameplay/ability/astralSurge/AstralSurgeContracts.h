#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::AstralSurge
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.AstralSurge.Basic";
	};

	inline const vega::GameplayTag CategoryTag = vega::GameplayTags::Ability::Offense;
	inline const vega::GameplayTag FamilyTag = vega::GameplayTags::Ability::Family::AstralSurge;

	struct State
	{
		inline static const vega::GameplayTag Focusing =
			vega::GameplayTags::State::Ability::AstralSurge::Focusing;
	};

	struct Event
	{
		inline static const vega::GameplayTag Started =
			vega::GameplayTags::Event::Ability::AstralSurge::Started;
		inline static const vega::GameplayTag Fired =
			vega::GameplayTags::Event::Ability::AstralSurge::Fired;
		inline static const vega::GameplayTag Ended =
			vega::GameplayTags::Event::Ability::AstralSurge::Ended;
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.AstralSurge.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.AstralSurge.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.AstralSurge.Projectile.ProjectileSpeed"
			};
			inline static const sas::AttributeId MinimumDamageMultiplier{
				"AbilityActor.AstralSurge.Projectile.MinimumDamageMultiplier"
			};
		};
	};

	struct Attribute
	{
		// The wave width is shared area geometry. The projectile converts it to
		// half-width for swept collision, so JSON owns one unambiguous full width.
		inline static const sas::AttributeId ProjectileWidth = vega::AreaAttributeIds::Width;
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId PierceDamageLoss = vega::CommonAttributeIds::PierceDamageLoss;
	};
}
