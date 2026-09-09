#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag LanceDrive{ "Ability.Offense.LanceDrive" };
}

namespace ly::GameplayTags::State::Ability::LanceDrive
{
	inline const GameplayTag Active{ "State.Ability.LanceDrive.Active" };
}

namespace ly::GameplayTags::Event::Ability::LanceDrive
{
	inline const GameplayTag Started{ "Event.Ability.LanceDrive.Started" };
	inline const GameplayTag Ended{ "Event.Ability.LanceDrive.Ended" };
}
