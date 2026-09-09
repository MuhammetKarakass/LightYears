#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag TimeSlip{ "Ability.Defense.TimeSlip" };
}

namespace ly::GameplayTags::State::Ability::TimeSlip
{
	inline const GameplayTag Active{ "State.Ability.TimeSlip.Active" };
}

namespace ly::GameplayTags::Event::Ability::TimeSlip
{
	inline const GameplayTag Started{ "Event.Ability.TimeSlip.Started" };
	inline const GameplayTag Ended{ "Event.Ability.TimeSlip.Ended" };
}
