#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag LanceDrive{ "Ability.Movement.LanceDrive" };
}

namespace vega::GameplayTags::State::Ability::LanceDrive
{
	inline const GameplayTag Active{ "State.Ability.LanceDrive.Active" };
}

namespace vega::GameplayTags::Event::Ability::LanceDrive
{
	inline const GameplayTag Started{ "Event.Ability.LanceDrive.Started" };
	inline const GameplayTag Ended{ "Event.Ability.LanceDrive.Ended" };
}
