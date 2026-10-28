#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ShieldHarvest
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.ShieldHarvest.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Defense };
	inline const vega::GameplayTag FamilyTag{ vega::GameplayTags::Ability::Family::ShieldHarvest };

	struct State
	{
		inline static const vega::GameplayTag Focusing{
			vega::GameplayTags::State::Ability::ShieldHarvest::Focusing
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::ShieldHarvest::Started
		};
		inline static const vega::GameplayTag Harvested{
			vega::GameplayTags::Event::Ability::ShieldHarvest::Harvested
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::ShieldHarvest::Ended
		};
	};

	struct Effect
	{
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	struct Attribute
	{
		// Radius is a common spatial concept; the ability only aliases it.
		inline static const sas::AttributeId Radius = vega::CommonAttributeIds::Radius;
		inline static const sas::AttributeId ShieldPerEnemy{
			"Ability.Defense.ShieldHarvest.ShieldPerEnemy"
		};
	};
}
