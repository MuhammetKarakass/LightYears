#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Stable family identity shared by content, behavior validation and runtime.
	inline const GameplayTag VectorSync{ "Ability.Movement.VectorSync" };
}

namespace ly::GameplayTags::State::Ability::VectorSync
{
	inline const GameplayTag Active{ "State.Ability.VectorSync.Active" };
}
