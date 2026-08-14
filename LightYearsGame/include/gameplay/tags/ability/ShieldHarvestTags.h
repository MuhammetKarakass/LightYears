#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag ShieldHarvest{ "Ability.Defense.ShieldHarvest" };
}

namespace ly::GameplayTags::State::Ability::ShieldHarvest
{
	inline const GameplayTag Focusing{ "State.Ability.ShieldHarvest.Focusing" };
}

namespace ly::GameplayTags::Event::Ability::ShieldHarvest
{
	inline const GameplayTag Started{ "Event.Ability.ShieldHarvest.Started" };
	inline const GameplayTag Harvested{ "Event.Ability.ShieldHarvest.Harvested" };
	inline const GameplayTag Ended{ "Event.Ability.ShieldHarvest.Ended" };
}
