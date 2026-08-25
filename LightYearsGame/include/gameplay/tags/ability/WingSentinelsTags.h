#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag WingSentinels{ "Ability.Offense.WingSentinels" };
}

namespace ly::GameplayTags::State::Ability::WingSentinels
{
	inline const GameplayTag Active{ "State.Ability.WingSentinels.Active" };
}

namespace ly::GameplayTags::Event::Ability::WingSentinels
{
	inline const GameplayTag Started{ "Event.Ability.WingSentinels.Started" };
	inline const GameplayTag Ended{ "Event.Ability.WingSentinels.Ended" };
}
