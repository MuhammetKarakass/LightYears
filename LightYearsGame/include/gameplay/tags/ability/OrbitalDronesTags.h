#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	// The family tag is the stable identity shared by the content definition,
	// behavior registry, progression rules, and future runtime implementation.
	inline const GameplayTag OrbitalDrones{ "Ability.Offense.OrbitalDrones" };
}

namespace vega::GameplayTags::State::Ability::OrbitalDrones
{
	inline const GameplayTag Active{ "State.Ability.OrbitalDrones.Active" };
}

namespace vega::GameplayTags::Event::Ability::OrbitalDrones
{
	inline const GameplayTag Started{ "Event.Ability.OrbitalDrones.Started" };
	inline const GameplayTag Ended{ "Event.Ability.OrbitalDrones.Ended" };
}
