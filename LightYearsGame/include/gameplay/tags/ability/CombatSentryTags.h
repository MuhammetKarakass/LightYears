#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag CombatSentry{
		"Ability.Offense.CombatSentry"
	};
}

namespace ly::GameplayTags::Event::Ability::CombatSentry
{
	inline const GameplayTag Spawned{ "Event.Ability.CombatSentry.Spawned" };
	inline const GameplayTag Ended{ "Event.Ability.CombatSentry.Ended" };
}
