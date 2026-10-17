#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag EnergySpear{ "Ability.Movement.EnergySpear" };
}

namespace vega::GameplayTags::State::Ability::EnergySpear
{
	inline const GameplayTag Focusing{ "State.Ability.EnergySpear.Focusing" };
	inline const GameplayTag Traversing{ "State.Ability.EnergySpear.Traversing" };
}

namespace vega::GameplayTags::Event::Ability::EnergySpear
{
	inline const GameplayTag Started{ "Event.Ability.EnergySpear.Started" };
	inline const GameplayTag MaxDistanceReached{
		"Event.Ability.EnergySpear.MaxDistanceReached"
	};
	inline const GameplayTag Released{ "Event.Ability.EnergySpear.Released" };
	inline const GameplayTag Ended{ "Event.Ability.EnergySpear.Ended" };
}
