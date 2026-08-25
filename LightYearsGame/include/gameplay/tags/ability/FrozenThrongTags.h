#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag FrozenThrong{
		"Ability.Offense.FrozenThrong"
	};
}

namespace ly::GameplayTags::State::Ability::FrozenThrong
{
	inline const GameplayTag Active{
		"State.Ability.FrozenThrong.Active"
	};
}

namespace ly::GameplayTags::Event::Ability::FrozenThrong
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
