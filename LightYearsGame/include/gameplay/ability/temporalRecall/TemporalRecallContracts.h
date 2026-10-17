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

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Defense };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::TemporalRecall
	};

	struct State
	{
		inline static const vega::GameplayTag Focusing{
			vega::GameplayTags::State::Ability::TemporalRecall::Focusing
		};
		inline static const vega::GameplayTag Rewinding{
			vega::GameplayTags::State::Ability::TemporalRecall::Rewinding
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::TemporalRecall::Started
		};
		inline static const vega::GameplayTag RewindStarted{
			vega::GameplayTags::Event::Ability::TemporalRecall::RewindStarted
		};
		inline static const vega::GameplayTag Completed{
			vega::GameplayTags::Event::Ability::TemporalRecall::Completed
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::TemporalRecall::Ended
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
		inline static const sas::AttributeId BaseRecovery{
			"Ability.Defense.TemporalRecall.BaseRecovery"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Defense.TemporalRecall.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthRecoveryScale{
			"Ability.Defense.TemporalRecall.MaxHealthRecoveryScale"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Defense.TemporalRecall.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerRecoveryScale{
			"Ability.Defense.TemporalRecall.EnergyPowerRecoveryScale"
		};

		// Explicit aliases document that abilities scale from canonical owner stats,
		// never from derived Ship.Shield.Max or Afterburner capacity values.
		inline static const sas::AttributeId HealthScalingSource =
			vega::OwnerAttributeIds::MaxHealth;
		inline static const sas::AttributeId ShieldScalingSource =
			vega::OwnerAttributeIds::EnergyPower;
	};
}
