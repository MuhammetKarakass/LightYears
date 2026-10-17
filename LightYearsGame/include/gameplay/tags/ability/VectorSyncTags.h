#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	// Stable family identity shared by content, behavior validation and runtime.
	inline const GameplayTag VectorSync{ "Ability.Movement.VectorSync" };
}

namespace vega::GameplayTags::State::Ability::VectorSync
{
	inline const GameplayTag Active{ "State.Ability.VectorSync.Active" };
}
