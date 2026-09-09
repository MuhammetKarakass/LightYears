#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Family tags identify content/runtime ownership; they are not per-feature
	// tuning tags and therefore stay one stable leaf per ability family.
	inline const GameplayTag InertialWake{ "Ability.Offense.InertialWake" };
}

namespace ly::GameplayTags::State::Ability::InertialWake
{
	inline const GameplayTag Active{ "State.Ability.InertialWake.Active" };
}

namespace ly::GameplayTags::Event::Ability::InertialWake
{
	inline const GameplayTag Started{ "Event.Ability.InertialWake.Started" };
	inline const GameplayTag Ended{ "Event.Ability.InertialWake.Ended" };
}
