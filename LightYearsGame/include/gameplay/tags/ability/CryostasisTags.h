#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag Cryostasis{ "Ability.Defense.Cryostasis" };
}

namespace vega::GameplayTags::State::Ability::Cryostasis
{
	inline const GameplayTag Active{ "State.Ability.Cryostasis.Active" };
}

namespace vega::GameplayTags::State::Effect::Defense::Cryostasis
{
	inline const GameplayTag IceShell{ "State.Effect.Defense.Cryostasis.IceShell" };
}

namespace vega::GameplayTags::Event::Ability::Cryostasis
{
	inline const GameplayTag Started{ "Event.Ability.Cryostasis.Started" };
	inline const GameplayTag IceBroken{ "Event.Ability.Cryostasis.IceBroken" };
	inline const GameplayTag ManuallyEnded{ "Event.Ability.Cryostasis.ManuallyEnded" };
	inline const GameplayTag Ended{ "Event.Ability.Cryostasis.Ended" };
}
