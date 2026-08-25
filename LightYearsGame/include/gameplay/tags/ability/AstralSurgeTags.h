#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag AstralSurge{ "Ability.Offense.AstralSurge" };
}

namespace ly::GameplayTags::State::Ability::AstralSurge
{
	inline const GameplayTag Focusing{ "State.Ability.AstralSurge.Focusing" };
}

namespace ly::GameplayTags::Event::Ability::AstralSurge
{
	inline const GameplayTag Started{ "Event.Ability.AstralSurge.Started" };
	inline const GameplayTag Fired{ "Event.Ability.AstralSurge.Fired" };
	inline const GameplayTag Ended{ "Event.Ability.AstralSurge.Ended" };
}
