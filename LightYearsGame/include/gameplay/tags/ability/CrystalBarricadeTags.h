#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag CrystalBarricade{ "Ability.Defense.CrystalBarricade" };
}

namespace ly::GameplayTags::Event::Ability::CrystalBarricade
{
	inline const GameplayTag Placed{ "Event.Ability.CrystalBarricade.Placed" };
	inline const GameplayTag Ended{ "Event.Ability.CrystalBarricade.Ended" };
}
