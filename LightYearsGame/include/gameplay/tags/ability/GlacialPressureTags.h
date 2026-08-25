#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag GlacialPressure{
		"Ability.Offense.GlacialPressure"
	};
}

namespace ly::GameplayTags::State::Ability::GlacialPressure
{
	inline const GameplayTag Focusing{
		"State.Ability.GlacialPressure.Focusing"
	};
	inline const GameplayTag Pushing{
		"State.Ability.GlacialPressure.Pushing"
	};
}

namespace ly::GameplayTags::Event::Ability::GlacialPressure
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
