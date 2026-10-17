#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag PhaseDrift{ "Ability.Movement.PhaseDrift" };
}

namespace vega::GameplayTags::State::Ability::PhaseDrift
{
	inline const GameplayTag Active{ "State.Ability.PhaseDrift.Active" };
}

namespace vega::GameplayTags::Event::Ability::PhaseDrift
{
	inline const GameplayTag Started{ "Event.Ability.PhaseDrift.Started" };
	inline const GameplayTag BrokenByAction{ "Event.Ability.PhaseDrift.BrokenByAction" };
	inline const GameplayTag Completed{ "Event.Ability.PhaseDrift.Completed" };
	inline const GameplayTag Ended{ "Event.Ability.PhaseDrift.Ended" };
}
