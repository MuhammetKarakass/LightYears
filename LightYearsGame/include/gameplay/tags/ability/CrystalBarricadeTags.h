#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag CrystalBarricade{ "Ability.Defense.CrystalBarricade" };
}

namespace vega::GameplayTags::Event::Ability::CrystalBarricade
{
	inline const GameplayTag Placed{ "Event.Ability.CrystalBarricade.Placed" };
	inline const GameplayTag Ended{ "Event.Ability.CrystalBarricade.Ended" };
}
