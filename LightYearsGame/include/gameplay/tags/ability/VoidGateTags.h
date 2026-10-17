#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag VoidGate{ "Ability.Utility.VoidGate" };
}

namespace vega::GameplayTags::State::Ability::VoidGate
{
	inline const GameplayTag WaitingForPortalB{
		"State.Ability.VoidGate.WaitingForPortalB"
	};
	inline const GameplayTag Active{ "State.Ability.VoidGate.Active" };
}

namespace vega::GameplayTags::Event::Ability::VoidGate
{
	inline const GameplayTag Started{ "Event.Ability.VoidGate.Started" };
	inline const GameplayTag PortalAPlaced{ "Event.Ability.VoidGate.PortalAPlaced" };
	inline const GameplayTag PortalBPlaced{ "Event.Ability.VoidGate.PortalBPlaced" };
	inline const GameplayTag TransferStarted{ "Event.Ability.VoidGate.TransferStarted" };
	inline const GameplayTag TransferCompleted{ "Event.Ability.VoidGate.TransferCompleted" };
	inline const GameplayTag Ended{ "Event.Ability.VoidGate.Ended" };
}
