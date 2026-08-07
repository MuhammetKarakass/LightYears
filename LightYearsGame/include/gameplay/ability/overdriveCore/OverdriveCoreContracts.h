#pragma once

#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData::OverdriveCore
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.OverdriveCore.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.OverdriveCore" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Offense.OverdriveCore" };

	// These state leaves describe Overdrive's future lifecycle. Shared input
	// blocking stays in GameplayTagSchema so all abilities consume one rule.
	struct State
	{
		inline static const ly::GameplayTag Firing{ "State.Ability.OverdriveCore.Firing" };
		inline static const ly::GameplayTag AttackSpeedBoost{
			"State.Ability.OverdriveCore.AttackSpeedBoost"
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			"Event.Ability.OverdriveCore.Start"
		};
		inline static const ly::GameplayTag Ended{
			"Event.Ability.OverdriveCore.End"
		};
		inline static const ly::GameplayTag AttackSpeedBoostEnded{
			"Event.Ability.OverdriveCore.AttackSpeedBoostEnd"
		};
	};
}
