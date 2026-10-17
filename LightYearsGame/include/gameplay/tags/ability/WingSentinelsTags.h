#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag WingSentinels{ "Ability.Offense.WingSentinels" };
}

namespace vega::GameplayTags::State::Ability::WingSentinels
{
	inline const GameplayTag Active{ "State.Ability.WingSentinels.Active" };
}

namespace vega::GameplayTags::Event::Ability::WingSentinels
{
	inline const GameplayTag Started{ "Event.Ability.WingSentinels.Started" };
	inline const GameplayTag Ended{ "Event.Ability.WingSentinels.Ended" };
}
