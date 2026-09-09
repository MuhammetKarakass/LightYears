#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag Blastback{ "Ability.Offense.Blastback" };
}

namespace ly::GameplayTags::State::Ability::Blastback
{
	inline const GameplayTag Focusing{ "State.Ability.Blastback.Focusing" };
}

namespace ly::GameplayTags::Event::Ability::Blastback
{
	inline const GameplayTag Started{ "Event.Ability.Blastback.Started" };
	inline const GameplayTag Blasted{ "Event.Ability.Blastback.Blasted" };
	inline const GameplayTag Ended{ "Event.Ability.Blastback.Ended" };
}
