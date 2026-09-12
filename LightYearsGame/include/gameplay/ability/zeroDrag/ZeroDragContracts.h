#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ZeroDrag
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.ZeroDrag.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityMovement };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ZeroDrag" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::ZeroDrag };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::ZeroDrag::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::ZeroDrag::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::ZeroDrag::Ended
		};
	};

	// These values are ability-scoped runtime attributes. They stay in the
	// ability definition instead of the owner's global AttributeSystem because
	// they describe Zero Drag's own duration and release balance.
	struct Attribute
	{
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Movement.ZeroDrag.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerDurationPerPoint{
			"Ability.Movement.ZeroDrag.EnergyPowerDurationPerPoint"
		};
		inline static const sas::AttributeId ThrustBonus{
			"Ability.Movement.ZeroDrag.ThrustBonus"
		};
		inline static const sas::AttributeId NormalizationDuration{
			"Ability.Movement.ZeroDrag.NormalizationDuration"
		};
	};
}
