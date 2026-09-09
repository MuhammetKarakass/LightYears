#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag EmberSwarm{ "Ability.Offense.EmberSwarm" };
}

namespace ly::GameplayTags::State::Ability::EmberSwarm
{
	inline const GameplayTag Active{ "State.Ability.EmberSwarm.Active" };
}

namespace ly::GameplayTags::Event::Ability::EmberSwarm
{
	inline const GameplayTag Started{ "Event.Ability.EmberSwarm.Started" };
	inline const GameplayTag Ended{ "Event.Ability.EmberSwarm.Ended" };
}
