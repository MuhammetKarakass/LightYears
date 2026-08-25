#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag VoidGate{ "Ability.Utility.VoidGate" };
}

namespace ly::GameplayTags::State::Ability::VoidGate
{
	inline const GameplayTag WaitingForPortalB{
		"State.Ability.VoidGate.WaitingForPortalB"
	};
	inline const GameplayTag Active{ "State.Ability.VoidGate.Active" };
}

namespace ly::GameplayTags::Event::Ability::VoidGate
{
	inline const GameplayTag Started{ "Event.Ability.VoidGate.Started" };
	inline const GameplayTag PortalAPlaced{ "Event.Ability.VoidGate.PortalAPlaced" };
	inline const GameplayTag PortalBPlaced{ "Event.Ability.VoidGate.PortalBPlaced" };
	inline const GameplayTag TransferStarted{ "Event.Ability.VoidGate.TransferStarted" };
	inline const GameplayTag TransferCompleted{ "Event.Ability.VoidGate.TransferCompleted" };
	inline const GameplayTag Ended{ "Event.Ability.VoidGate.Ended" };
}
