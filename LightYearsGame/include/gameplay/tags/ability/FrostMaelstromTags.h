#pragma once

#include "framework/Core.h"

namespace ly::GameplayTags
{
	namespace Ability::Family
	{
		inline const GameplayTag FrostMaelstrom{
			"Ability.Control.FrostMaelstrom"
		};
	}

	namespace State::Ability::FrostMaelstrom
	{
		inline const GameplayTag Active{
			"State.Ability.FrostMaelstrom.Active"
		};
		inline const GameplayTag Controlling{
			"State.Ability.FrostMaelstrom.Controlling"
		};
	}

	namespace Event::Ability::FrostMaelstrom
	{
		inline const GameplayTag Started{
			"Event.Ability.FrostMaelstrom.Started"
		};
		inline const GameplayTag Tick{
			"Event.Ability.FrostMaelstrom.Tick"
		};
		inline const GameplayTag Released{
			"Event.Ability.FrostMaelstrom.Released"
		};
		inline const GameplayTag Ended{
			"Event.Ability.FrostMaelstrom.Ended"
		};
	}
}
