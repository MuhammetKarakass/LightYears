#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag ReturnProtocol{ "Ability.Defense.ReturnProtocol" };
}

namespace ly::GameplayTags::State::Ability::ReturnProtocol
{
	inline const GameplayTag Active{ "State.Ability.ReturnProtocol.Active" };
}

namespace ly::GameplayTags::Event::Ability::ReturnProtocol
{
	inline const GameplayTag Started{ "Event.Ability.ReturnProtocol.Started" };
	inline const GameplayTag Reflected{ "Event.Ability.ReturnProtocol.Reflected" };
	inline const GameplayTag Ended{ "Event.Ability.ReturnProtocol.Ended" };
}
