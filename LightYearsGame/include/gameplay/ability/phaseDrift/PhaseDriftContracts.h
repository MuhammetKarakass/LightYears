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
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.PhaseDrift" };
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

	// These are ability-local runtime values. They are intentionally not JSON
	// "settings": the behavior resolves them with owner attributes and level.
	struct Attribute
	{
		inline static const sas::AttributeId MaximumMobilityDurationBonus{
			"Ability.Movement.PhaseDrift.MaximumMobilityDurationBonus"
		};
		inline static const sas::AttributeId MobilityScale{
			"Ability.Movement.PhaseDrift.MobilityScale"
		};
		inline static const sas::AttributeId MovementSpeedBonus{
			"Ability.Movement.PhaseDrift.MovementSpeedBonus"
		};
		inline static const sas::AttributeId ShieldRegenBonus{
			"Ability.Movement.PhaseDrift.ShieldRegenBonus"
		};
		inline static const sas::AttributeId AfterburnerRegenBonus{
			"Ability.Movement.PhaseDrift.AfterburnerRegenBonus"
		};
		inline static const sas::AttributeId EnergyScale{
			"Ability.Movement.PhaseDrift.EnergyScale"
		};
		inline static const sas::AttributeId MaximumEnergyShieldBonus{
			"Ability.Movement.PhaseDrift.MaximumEnergyShieldBonus"
		};
		inline static const sas::AttributeId MaximumEnergyAfterburnerBonus{
			"Ability.Movement.PhaseDrift.MaximumEnergyAfterburnerBonus"
		};
	};

	struct Effect
	{
		inline static constexpr char MovementBoostId[] =
			"Effect.PhaseDrift.MovementBoost";
		inline static constexpr char ShieldRecoveryId[] =
			"Effect.PhaseDrift.ShieldRecovery";
		inline static constexpr char AfterburnerRecoveryId[] =
			"Effect.PhaseDrift.AfterburnerRecovery";
	};
}
