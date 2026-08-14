#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag RelayPrism{ "Ability.Utility.RelayPrism" };
}

namespace ly::GameplayTags::State::Ability::RelayPrism
{
	inline const GameplayTag Active{ "State.Ability.RelayPrism.Active" };
}

namespace ly::GameplayTags::Event::Ability::RelayPrism
{
	inline const GameplayTag Started{ "Event.Ability.RelayPrism.Started" };
	inline const GameplayTag Captured{ "Event.Ability.RelayPrism.Captured" };
	inline const GameplayTag Ended{ "Event.Ability.RelayPrism.Ended" };
}
