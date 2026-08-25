#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag NanoPlague{ "Ability.Offense.NanoPlague" };
}

namespace ly::GameplayTags::State::Effect::NanoPlague
{
	// This is a target state, not an effect ID. The persistent Nano Plague
	// controller owns its lifetime and removes it when infection ends.
	inline const GameplayTag Infected{ "State.Effect.NanoPlague.Infected" };
}
