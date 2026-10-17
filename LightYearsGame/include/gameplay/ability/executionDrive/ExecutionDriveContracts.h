#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ExecutionDrive
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.ExecutionDrive.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Offense };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::ExecutionDrive
	};

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::ExecutionDrive::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::ExecutionDrive::Started
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::ExecutionDrive::Ended
		};
	};

	struct Effect
	{
		inline static constexpr char AttackPowerId[] =
			"Effect.Offense.ExecutionDrive.AttackPower.Basic";
 	};

	struct Attribute
	{
		inline static const sas::AttributeId FlatAttackPowerBonus{
			"Ability.Offense.ExecutionDrive.FlatAttackPowerBonus"
		};
		inline static const sas::AttributeId AttackPowerScale{
			"Ability.Offense.ExecutionDrive.AttackPowerScale"
		};
		inline static const sas::AttributeId MoveSpeedBonus{
			"Ability.Offense.ExecutionDrive.MoveSpeedBonus"
		};
		inline static const sas::AttributeId Range = vega::CommonAttributeIds::Range;
	};
}
