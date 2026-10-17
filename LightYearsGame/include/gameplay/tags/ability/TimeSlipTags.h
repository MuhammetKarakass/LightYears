#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag TimeSlip{ "Ability.Defense.TimeSlip" };
}

namespace vega::GameplayTags::State::Ability::TimeSlip
{
	inline const GameplayTag Active{ "State.Ability.TimeSlip.Active" };
}

namespace vega::GameplayTags::Event::Ability::TimeSlip
{
	inline const GameplayTag Started{ "Event.Ability.TimeSlip.Started" };
	inline const GameplayTag Ended{ "Event.Ability.TimeSlip.Ended" };
}
