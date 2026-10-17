#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag GlacialPressure{
		"Ability.Offense.GlacialPressure"
	};
}

namespace vega::GameplayTags::State::Ability::GlacialPressure
{
	inline const GameplayTag Focusing{
		"State.Ability.GlacialPressure.Focusing"
	};
	inline const GameplayTag Pushing{
		"State.Ability.GlacialPressure.Pushing"
	};
}

namespace vega::GameplayTags::Event::Ability::GlacialPressure
{
	inline const GameplayTag Started{
		"Event.Ability.GlacialPressure.Started"
	};
	inline const GameplayTag Blasted{
		"Event.Ability.GlacialPressure.Blasted"
	};
	inline const GameplayTag Collision{
		"Event.Ability.GlacialPressure.Collision"
	};
	inline const GameplayTag Ended{
		"Event.Ability.GlacialPressure.Ended"
	};
}
