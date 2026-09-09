#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Stable family identity used by content validation and runtime queries.
	inline const GameplayTag ZeroDrag{ "Ability.Movement.ZeroDrag" };
}

namespace ly::GameplayTags::State::Ability::ZeroDrag
{
	inline const GameplayTag Active{ "State.Ability.ZeroDrag.Active" };
}

namespace ly::GameplayTags::Event::Ability::ZeroDrag
{
	inline const GameplayTag Started{ "Event.Ability.ZeroDrag.Started" };
	inline const GameplayTag Ended{ "Event.Ability.ZeroDrag.Ended" };
}
