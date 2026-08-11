#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag OverdriveCore{ "Ability.Offense.OverdriveCore" };
}

namespace ly::GameplayTags::State::Ability::OverdriveCore
{
	namespace RocketLaunch
	{
		inline const GameplayTag Active{
			"State.Ability.OverdriveCore.RocketLaunch.Active"
		};
	}

	namespace AttackSpeedBoost
	{
		inline const GameplayTag Active{
			"State.Ability.OverdriveCore.AttackSpeedBoost.Active"
		};
	}
}

namespace ly::GameplayTags::Event::Ability::OverdriveCore
{
	inline const GameplayTag RocketLaunchStarted{
		"Event.Ability.OverdriveCore.RocketLaunch.Start"
	};
	inline const GameplayTag RocketLaunchEnded{
		"Event.Ability.OverdriveCore.RocketLaunch.End"
	};
	inline const GameplayTag AttackSpeedBoostStarted{
		"Event.Ability.OverdriveCore.AttackSpeedBoost.Start"
	};
	inline const GameplayTag AttackSpeedBoostEnded{
		"Event.Ability.OverdriveCore.AttackSpeedBoost.End"
	};
}

namespace ly::GameplayTags::State::Effect::OverdriveCore
{
	// Effects may grant only State.Effect.* or Status.* tags. This is therefore
	// separate from the ability-owned State.Ability.* lifecycle tags above.
	inline const GameplayTag AttackSpeedBoost{
		"State.Effect.Offense.OverdriveCore.AttackSpeedBoost"
	};
}
