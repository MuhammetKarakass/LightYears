#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/content/NumericSettingContract.h"

namespace AbilityData::HullShock
{
	struct Evolve
	{
		inline static constexpr char DirectedDischarge[] = "Ability.Evolve.HullShock.DirectedDischarge";
		inline static constexpr char ReactiveCapacitor[] = "Ability.Evolve.HullShock.ReactiveCapacitor";
		inline static constexpr char AftershockResonance[] = "Ability.Evolve.HullShock.AftershockResonance";
	};
	struct Effect
	{
		inline static constexpr char ReactiveCapacitor[] = "Effect.Defense.HullShock.ReactiveCapacitor.Basic";
	};
	struct Setting
	{
		inline static constexpr char ConeAngleDegrees[] = "coneAngleDegrees";
		inline static constexpr char DirectedDamageMultiplier[] = "directedDamageMultiplier";
		inline static constexpr char RecordedDamageShieldFraction[] = "recordedDamageShieldFraction";
		inline static constexpr char MaxHealthShieldCapFraction[] = "maxHealthShieldCapFraction";
		inline static constexpr char AftershockDelay[] = "aftershockDelay";
		inline static constexpr char AftershockDamageFraction[] = "aftershockDamageFraction";
		inline static constexpr char AftershockElectricStacks[] = "aftershockElectricStacks";
		inline static const vega::content::NumericSettingContract Contract{
			{ ConeAngleDegrees, DirectedDamageMultiplier, RecordedDamageShieldFraction, MaxHealthShieldCapFraction, AftershockDelay, AftershockDamageFraction, AftershockElectricStacks }, {}
		};
	};
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.HullShock.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Offense };
	inline const vega::GameplayTag FamilyTag{ vega::GameplayTags::Ability::Family::HullShock };

	struct State
	{
		inline static const vega::GameplayTag Focusing{
			vega::GameplayTags::State::Ability::HullShock::Focusing
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::HullShock::Started
		};
		inline static const vega::GameplayTag Discharged{
			vega::GameplayTags::Event::Ability::HullShock::Discharged
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::HullShock::Ended
		};
	};

	struct Attribute
	{
		// Radius and damage are shared combat concepts; the ability aliases them
		// instead of creating duplicate Common.* attribute IDs.
		inline static const sas::AttributeId Radius = vega::CommonAttributeIds::Radius;
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
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
	};
}
