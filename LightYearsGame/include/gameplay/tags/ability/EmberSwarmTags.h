#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag EmberSwarm{ "Ability.Offense.EmberSwarm" };
}

namespace vega::GameplayTags::State::Ability::EmberSwarm
{
	inline const GameplayTag Active{ "State.Ability.EmberSwarm.Active" };
}

namespace vega::GameplayTags::Event::Ability::EmberSwarm
{
	inline const GameplayTag Started{ "Event.Ability.EmberSwarm.Started" };
	inline const GameplayTag Ended{ "Event.Ability.EmberSwarm.Ended" };
}
