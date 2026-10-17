#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::EmberSwarm
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.EmberSwarm.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Offense };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::EmberSwarm
	};

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::EmberSwarm::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::EmberSwarm::Started
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::EmberSwarm::Ended
		};
	};
}
