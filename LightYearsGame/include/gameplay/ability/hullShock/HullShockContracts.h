#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

namespace AbilityData::HullShock
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.HullShock.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.HullShock" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::HullShock };

	struct State
	{
		inline static const ly::GameplayTag Focusing{
			ly::GameplayTags::State::Ability::HullShock::Focusing
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::HullShock::Started
		};
		inline static const ly::GameplayTag Discharged{
			ly::GameplayTags::Event::Ability::HullShock::Discharged
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::HullShock::Ended
		};
	};

	struct Attribute
	{
		// Radius and damage are shared combat concepts; the ability aliases them
		// instead of creating duplicate Common.* attribute IDs.
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId FullRadiusDuration{
			"Ability.Offense.HullShock.FullRadiusDuration"
		};
		inline static const sas::AttributeId MinimumChargeRadius{
			"Ability.Offense.HullShock.MinimumChargeRadius"
		};
		inline static const sas::AttributeId ElectricChargeThreshold{
			"Ability.Offense.HullShock.ElectricChargeThreshold"
		};
		inline static const sas::AttributeId MinimumChargeDamageMultiplier{
			"Ability.Offense.HullShock.MinimumChargeDamageMultiplier"
		};

		// These values belong to Hull Shock's balance contract. They are mapped
		// into the shared DamagePayload only at discharge; the ability definition
		// must not own Damage.* system attributes directly.
		inline static const sas::AttributeId ElectricDamageTakenMultiplierPerStack{
			"Ability.Offense.HullShock.ElectricDamageTakenMultiplierPerStack"
		};
		inline static const sas::AttributeId ElectricDuration{
			"Ability.Offense.HullShock.ElectricDuration"
		};
		inline static const sas::AttributeId ElectricMaxStacks{
			"Ability.Offense.HullShock.ElectricMaxStacks"
		};
	};
}
