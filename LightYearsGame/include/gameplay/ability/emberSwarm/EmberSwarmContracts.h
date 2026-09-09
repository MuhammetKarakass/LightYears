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

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.EmberSwarm" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::EmberSwarm
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::EmberSwarm::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::EmberSwarm::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::EmberSwarm::Ended
		};
	};
}
