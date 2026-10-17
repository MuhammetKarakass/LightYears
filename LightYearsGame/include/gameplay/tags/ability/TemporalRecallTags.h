#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag TemporalRecall{ "Ability.Defense.TemporalRecall" };
}

namespace vega::GameplayTags::State::Ability::TemporalRecall
{
	inline const GameplayTag Focusing{ "State.Ability.TemporalRecall.Focusing" };
	inline const GameplayTag Rewinding{ "State.Ability.TemporalRecall.Rewinding" };
}

namespace vega::GameplayTags::Event::Ability::TemporalRecall
{
	inline const GameplayTag Started{ "Event.Ability.TemporalRecall.Started" };
	inline const GameplayTag RewindStarted{
		"Event.Ability.TemporalRecall.RewindStarted"
	};
	inline const GameplayTag Completed{ "Event.Ability.TemporalRecall.Completed" };
	inline const GameplayTag Ended{ "Event.Ability.TemporalRecall.Ended" };
}
