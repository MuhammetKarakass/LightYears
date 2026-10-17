#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag FoldspaceArena{ "Ability.Utility.FoldspaceArena" };
}

namespace vega::GameplayTags::State::Ability::FoldspaceArena
{
	inline const GameplayTag Active{ "State.Ability.FoldspaceArena.Active" };
	inline const GameplayTag CancelAvailable{
		"State.Ability.FoldspaceArena.CancelAvailable"
	};
}

namespace vega::GameplayTags::Event::Ability::FoldspaceArena
{
	inline const GameplayTag Started{ "Event.Ability.FoldspaceArena.Start" };
	inline const GameplayTag CancelAvailable{
		"Event.Ability.FoldspaceArena.CancelAvailable"
	};
	inline const GameplayTag Ended{ "Event.Ability.FoldspaceArena.End" };
}
