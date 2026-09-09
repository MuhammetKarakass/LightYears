#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	inline const GameplayTag ReclaimerProtocol{
		"Ability.Defense.ReclaimerProtocol"
	};
}

namespace ly::GameplayTags::State::Ability::ReclaimerProtocol
{
	inline const GameplayTag Active{
		"State.Ability.ReclaimerProtocol.Active"
	};
}

namespace ly::GameplayTags::Event::Ability::ReclaimerProtocol
{
	inline const GameplayTag Started{
		"Event.Ability.ReclaimerProtocol.Started"
	};
	inline const GameplayTag RepairKitSpawned{
		"Event.Ability.ReclaimerProtocol.RepairKitSpawned"
	};
	inline const GameplayTag Ended{
		"Event.Ability.ReclaimerProtocol.Ended"
	};
}