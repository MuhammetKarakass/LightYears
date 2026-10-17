#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag IroncladProtocol{ "Ability.Defense.IroncladProtocol" };
}

namespace vega::GameplayTags::State::Ability::IroncladProtocol
{
	inline const GameplayTag Active{ "State.Ability.IroncladProtocol.Active" };
	inline const GameplayTag CancelAvailable{ "State.Ability.IroncladProtocol.CancelAvailable" };
}

namespace vega::GameplayTags::Event::Ability::IroncladProtocol
{
	inline const GameplayTag Started{ "Event.Ability.IroncladProtocol.Start" };
	inline const GameplayTag CancelAvailable{ "Event.Ability.IroncladProtocol.CancelAvailable" };
	inline const GameplayTag Ended{ "Event.Ability.IroncladProtocol.End" };
}
