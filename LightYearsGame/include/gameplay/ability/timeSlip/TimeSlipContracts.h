#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::TimeSlip
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.TimeSlip.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.TimeSlip" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::TimeSlip
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::TimeSlip::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::TimeSlip::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::TimeSlip::Ended
		};
	};

	struct Attribute
	{
		// This shared value controls hostile actors and every projectile actor.
		// Keeping one multiplier guarantees that all projectile families receive
		// the same temporal slowdown instead of drifting into separate balance
		// values. The ability still reads the canonical Owner.Energy.Max below.
		inline static const sas::AttributeId GameplayTimeMultiplier{
			"Ability.Defense.TimeSlip.GameplayTimeMultiplier"
		};
		inline static const sas::AttributeId PrimaryFireRateMultiplier{
			"Ability.Defense.TimeSlip.PrimaryFireRateMultiplier"
		};
		inline static const sas::AttributeId EnergyMaxReference{
			"Ability.Defense.TimeSlip.EnergyMaxReference"
		};
		inline static const sas::AttributeId EnergyMaxDurationScale{
			"Ability.Defense.TimeSlip.EnergyMaxDurationScale"
		};

		// Explicit alias documents the only owner stat used by Time Slip scaling.
		inline static const sas::AttributeId DurationScalingSource =
			ly::OwnerAttributeIds::EnergyMax;
	};
}
