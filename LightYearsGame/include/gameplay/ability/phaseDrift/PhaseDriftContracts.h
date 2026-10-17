#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::PhaseDrift
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.PhaseDrift.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTagSchema::AbilityMovement };
	inline const vega::GameplayTag FamilyTag{ vega::GameplayTags::Ability::Family::PhaseDrift };

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::PhaseDrift::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::PhaseDrift::Started
		};
		inline static const vega::GameplayTag BrokenByAction{
			vega::GameplayTags::Event::Ability::PhaseDrift::BrokenByAction
		};
		inline static const vega::GameplayTag Completed{
			vega::GameplayTags::Event::Ability::PhaseDrift::Completed
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::PhaseDrift::Ended
		};
	};

	struct Attribute
	{
		inline static const sas::AttributeId EPDurationScale{
			"Ability.Movement.PhaseDrift.EPDurationScale"
		};
		inline static const sas::AttributeId MovementSpeedBonus{
			"Ability.Movement.PhaseDrift.MovementSpeedBonus"
		};
	};
}
