#pragma once

#include "framework/Core.h"
#include "framework/GameplayTag.h"

namespace vega::GameplayTags::State::Effect::Control
{
	// Forced displacement (pull/drag). Targets resolve it through their control response like a stun.
	inline const GameplayTag Displaced{ "State.Effect.Control.Displaced" };
}

namespace vega
{
	class Actor;

	// Seconds a control effect of controlTag lasts on target after its control response: Immune and InterruptOnly targets
	// ignore it (0), Reduced targets get the response's shorter duration, non-combatants take it in full.
	float ResolveControlDuration(const Actor& target, const GameplayTag& controlTag, float seconds);
	// Applies the shared basic stun through the target's control response. Returns the applied seconds (0 when none).
	float ApplyControlStun(Actor& target, float seconds, Actor* source);
}
