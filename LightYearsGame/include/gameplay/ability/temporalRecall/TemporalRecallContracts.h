#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::TemporalRecall
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.TemporalRecall.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.TemporalRecall" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::TemporalRecall
	};

	struct State
	{
		inline static const ly::GameplayTag Focusing{
			ly::GameplayTags::State::Ability::TemporalRecall::Focusing
		};
		inline static const ly::GameplayTag Rewinding{
			ly::GameplayTags::State::Ability::TemporalRecall::Rewinding
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::TemporalRecall::Started
		};
		inline static const ly::GameplayTag RewindStarted{
			ly::GameplayTags::Event::Ability::TemporalRecall::RewindStarted
		};
		inline static const ly::GameplayTag Completed{
			ly::GameplayTags::Event::Ability::TemporalRecall::Completed
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::TemporalRecall::Ended
		};
	};

	struct Attribute
	{
		// These are family-local because their semantics are a temporal recall
		// contract, not generic duration or healing concepts shared by all abilities.
		inline static const sas::AttributeId RecallWindow{
			"Ability.Defense.TemporalRecall.RecallWindow"
		};
		inline static const sas::AttributeId FocusDuration{
			"Ability.Defense.TemporalRecall.FocusDuration"
		};
		inline static const sas::AttributeId RewindDuration{
			"Ability.Defense.TemporalRecall.RewindDuration"
		};
		inline static const sas::AttributeId PositiveRecoveryRatio{
			"Ability.Defense.TemporalRecall.PositiveRecoveryRatio"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Defense.TemporalRecall.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthRecoveryScale{
			"Ability.Defense.TemporalRecall.MaxHealthRecoveryScale"
		};
		inline static const sas::AttributeId EnergyMaxReference{
			"Ability.Defense.TemporalRecall.EnergyMaxReference"
		};
		inline static const sas::AttributeId EnergyMaxRecoveryScale{
			"Ability.Defense.TemporalRecall.EnergyMaxRecoveryScale"
		};
		inline static const sas::AttributeId OvercapHoldDuration{
			"Ability.Defense.TemporalRecall.OvercapHoldDuration"
		};
		inline static const sas::AttributeId OvercapDecayPerSecond{
			"Ability.Defense.TemporalRecall.OvercapDecayPerSecond"
		};

		// Explicit aliases document that abilities scale from canonical owner stats,
		// never from derived Ship.Shield.Max or Afterburner capacity values.
		inline static const sas::AttributeId HealthScalingSource =
			ly::OwnerAttributeIds::MaxHealth;
		inline static const sas::AttributeId ShieldScalingSource =
			ly::OwnerAttributeIds::EnergyMax;
	};
}
