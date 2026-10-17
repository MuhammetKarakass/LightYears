#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	// Stable family identity used by content validation and runtime queries.
	inline const GameplayTag ZeroDrag{ "Ability.Movement.ZeroDrag" };
}

namespace vega::GameplayTags::State::Ability::ZeroDrag
{
	inline const GameplayTag Active{ "State.Ability.ZeroDrag.Active" };
}

namespace vega::GameplayTags::Event::Ability::ZeroDrag
{
	inline const GameplayTag Started{ "Event.Ability.ZeroDrag.Started" };
	inline const GameplayTag Ended{ "Event.Ability.ZeroDrag.Ended" };
}
