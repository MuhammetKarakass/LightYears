#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag ExecutionDrive{
		"Ability.Offense.ExecutionDrive"
	};
}

namespace ly::GameplayTags::State::Ability::ExecutionDrive
{
	inline const GameplayTag Active{
		"State.Ability.ExecutionDrive.Active"
	};
}

namespace ly::GameplayTags::Event::Ability::ExecutionDrive
{
	inline const GameplayTag Started{
		"Event.Ability.ExecutionDrive.Started"
	};
	inline const GameplayTag Ended{
		"Event.Ability.ExecutionDrive.Ended"
	};
}

namespace ly::GameplayTags::State::Effect::Offense::ExecutionDrive
{
	inline const GameplayTag AttackPower{
		"State.Effect.Offense.ExecutionDrive.AttackPower"
	};
}
