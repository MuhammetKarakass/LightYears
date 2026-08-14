#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag HullShock{ "Ability.Offense.HullShock" };
}

namespace ly::GameplayTags::State::Ability::HullShock
{
	inline const GameplayTag Focusing{ "State.Ability.HullShock.Focusing" };
}

namespace ly::GameplayTags::Event::Ability::HullShock
{
	inline const GameplayTag Started{ "Event.Ability.HullShock.Started" };
	inline const GameplayTag Discharged{ "Event.Ability.HullShock.Discharged" };
	inline const GameplayTag Ended{ "Event.Ability.HullShock.Ended" };
}
