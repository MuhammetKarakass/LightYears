#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag ClosedCircuit{ "Ability.Defense.ClosedCircuit" };
}

namespace ly::GameplayTags::State::Ability::ClosedCircuit
{
	inline const GameplayTag Deploying{ "State.Ability.ClosedCircuit.Deploying" };
}

namespace ly::GameplayTags::Event::Ability::ClosedCircuit
{
	inline const GameplayTag Started{ "Event.Ability.ClosedCircuit.Start" };
	inline const GameplayTag Deployed{ "Event.Ability.ClosedCircuit.Deployed" };
	inline const GameplayTag Ended{ "Event.Ability.ClosedCircuit.End" };
}
