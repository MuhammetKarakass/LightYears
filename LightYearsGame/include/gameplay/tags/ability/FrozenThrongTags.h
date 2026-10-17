#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag FrozenThrong{
		"Ability.Offense.FrozenThrong"
	};
}

namespace vega::GameplayTags::State::Ability::FrozenThrong
{
	inline const GameplayTag Active{
		"State.Ability.FrozenThrong.Active"
	};
}

namespace vega::GameplayTags::Event::Ability::FrozenThrong
{
	inline const GameplayTag Started{
		"Event.Ability.FrozenThrong.Started"
	};
	inline const GameplayTag HuskSpawned{
		"Event.Ability.FrozenThrong.HuskSpawned"
	};
	inline const GameplayTag Ended{
		"Event.Ability.FrozenThrong.Ended"
	};
}
