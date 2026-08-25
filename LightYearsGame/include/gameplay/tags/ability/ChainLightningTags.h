#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags::Ability::Family
{
	// Family identity is shared by the content validator and the runtime
	// behavior registry. Keeping it in the common tag catalog prevents the
	// ability contract from inventing a second spelling for the same family.
	inline const GameplayTag ChainLightning{
		"Ability.Offense.ChainLightning"
	};
}

namespace ly::GameplayTags::State::Ability::ChainLightning
{
	inline const GameplayTag Active{
		"State.Ability.ChainLightning.Active"
	};
}

namespace ly::GameplayTags::Event::Ability::ChainLightning
{
	inline const GameplayTag Started{
		"Event.Ability.ChainLightning.Started"
	};
	inline const GameplayTag LinkHit{
		"Event.Ability.ChainLightning.LinkHit"
	};
	inline const GameplayTag Ended{
		"Event.Ability.ChainLightning.Ended"
	};
}
