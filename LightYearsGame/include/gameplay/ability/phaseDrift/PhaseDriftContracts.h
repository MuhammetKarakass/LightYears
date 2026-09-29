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

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityMovement };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::PhaseDrift };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::PhaseDrift::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::PhaseDrift::Started
		};
		inline static const ly::GameplayTag BrokenByAction{
			ly::GameplayTags::Event::Ability::PhaseDrift::BrokenByAction
		};
		inline static const ly::GameplayTag Completed{
			ly::GameplayTags::Event::Ability::PhaseDrift::Completed
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::PhaseDrift::Ended
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
