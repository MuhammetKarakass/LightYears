#pragma once

#include "framework/Core.h"

namespace vega::GameplayTags::Ability::Family
{
	inline const GameplayTag ReclaimerProtocol{
		"Ability.Defense.ReclaimerProtocol"
	};
}

namespace vega::GameplayTags::State::Ability::ReclaimerProtocol
{
	inline const GameplayTag Active{
		"State.Ability.ReclaimerProtocol.Active"
	};
}

namespace vega::GameplayTags::Event::Ability::ReclaimerProtocol
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