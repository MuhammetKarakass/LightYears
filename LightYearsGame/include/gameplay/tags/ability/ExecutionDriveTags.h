#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag ExecutionDrive{
		"Ability.Offense.ExecutionDrive"
	};
}

namespace vega::GameplayTags::State::Ability::ExecutionDrive
{
	inline const GameplayTag Active{
		"State.Ability.ExecutionDrive.Active"
	};
}

namespace vega::GameplayTags::Event::Ability::ExecutionDrive
{
	inline const GameplayTag Started{
		"Event.Ability.ExecutionDrive.Started"
	};
	inline const GameplayTag Ended{
		"Event.Ability.ExecutionDrive.Ended"
	};
}

namespace vega::GameplayTags::State::Effect::Offense::ExecutionDrive
{
	inline const GameplayTag AttackPower{
		"State.Effect.Offense.ExecutionDrive.AttackPower"
	};
}
