#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag IroncladProtocol{ "Ability.Defense.IroncladProtocol" };
}

namespace ly::GameplayTags::State::Ability::IroncladProtocol
{
	inline const GameplayTag Active{ "State.Ability.IroncladProtocol.Active" };
	inline const GameplayTag CancelAvailable{ "State.Ability.IroncladProtocol.CancelAvailable" };
}

namespace ly::GameplayTags::Event::Ability::IroncladProtocol
{
	inline const GameplayTag Started{ "Event.Ability.IroncladProtocol.Start" };
	inline const GameplayTag CancelAvailable{ "Event.Ability.IroncladProtocol.CancelAvailable" };
	inline const GameplayTag Ended{ "Event.Ability.IroncladProtocol.End" };
}
