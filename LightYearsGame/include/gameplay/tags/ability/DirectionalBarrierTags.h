#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag DirectionalBarrier{
		"Ability.Defense.DirectionalBarrier"
	};
}

namespace vega::GameplayTags::State::Effect::Defense::DirectionalBarrier
{
	// The effect owns the single persistent marker for the active barrier.
	// Directional protection and UI/VFX consumers can observe this tag without
	// adding a second ability-owned active tag with the same meaning.
	inline const GameplayTag Active{
		"State.Effect.Defense.DirectionalBarrier.Active"
	};
}

namespace vega::GameplayTags::Event::Ability::DirectionalBarrier
{
	inline const GameplayTag Started{
		"Event.Ability.DirectionalBarrier.Start"
	};
	inline const GameplayTag Ended{
		"Event.Ability.DirectionalBarrier.End"
	};
}
