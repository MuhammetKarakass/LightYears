#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// The family tag is the stable semantic identity shared by content,
	// attachments, progression rules and the C++ behavior registry.
	inline const GameplayTag NullPulse{ "Ability.Control.NullPulse" };
}

namespace ly::GameplayTags::State::Effect::Control
{
	// Gameplay effects grant state tags, so the runtime can consume Stun without
	// knowing which ability created it.
	inline const GameplayTag Stunned{ "State.Effect.Control.Stunned" };
	inline const GameplayTag Staggered{ "State.Effect.Control.Staggered" };
}

namespace ly::GameplayTags::Event::Ability::NullPulse
{
	inline const GameplayTag Activated{ "Event.Ability.NullPulse.Activated" };
	inline const GameplayTag ProjectilesCleared{ "Event.Ability.NullPulse.ProjectilesCleared" };
	inline const GameplayTag Completed{ "Event.Ability.NullPulse.Completed" };
}
