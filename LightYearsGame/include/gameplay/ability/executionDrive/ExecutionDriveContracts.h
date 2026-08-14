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

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ExecutionDrive" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::ExecutionDrive
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::ExecutionDrive::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::ExecutionDrive::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::ExecutionDrive::Ended
		};
	};

	struct Effect
	{
		inline static constexpr char AttackPowerId[] =
			"Effect.Offense.ExecutionDrive.AttackPower.Basic";
 	};

	struct Attribute
	{
		inline static const sas::AttributeId BaseAttackPowerBonus{
			"Ability.Offense.ExecutionDrive.BaseAttackPowerBonus"
		};
		inline static const sas::AttributeId AttackPowerPerStack{
			"Ability.Offense.ExecutionDrive.AttackPowerPerStack"
		};
		inline static const sas::AttributeId BaseChaseMovementBonus{
			"Ability.Offense.ExecutionDrive.BaseChaseMovementBonus"
		};
		inline static const sas::AttributeId ChaseMovementPerStack{
			"Ability.Offense.ExecutionDrive.ChaseMovementPerStack"
		};
		inline static const sas::AttributeId AttackPowerChaseScale{
			"Ability.Offense.ExecutionDrive.AttackPowerChaseScale"
		};
		inline static const sas::AttributeId TargetingRange =
			ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId DirectionThreshold{
			"Ability.Offense.ExecutionDrive.DirectionThreshold"
		};
	};
}
