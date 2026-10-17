#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag CombatSentry{
		"Ability.Offense.CombatSentry"
	};
}

namespace vega::GameplayTags::Event::Ability::CombatSentry
{
	inline const GameplayTag Spawned{ "Event.Ability.CombatSentry.Spawned" };
	inline const GameplayTag Ended{ "Event.Ability.CombatSentry.Ended" };
}
