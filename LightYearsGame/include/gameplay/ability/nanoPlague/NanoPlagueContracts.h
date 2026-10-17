#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::NanoPlague
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.NanoPlague.Basic";
	};

	inline const vega::GameplayTag CategoryTag = vega::GameplayTags::Ability::Offense;
	inline const vega::GameplayTag FamilyTag = vega::GameplayTags::Ability::Family::NanoPlague;

	struct State
	{
		inline static const vega::GameplayTag Infected =
			vega::GameplayTags::State::Effect::NanoPlague::Infected;
	};

	struct Attribute
	{
		// Shared output/delivery channels retain the project-wide meanings.
		inline static const sas::AttributeId& BaseTickDamage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId& Duration = vega::CommonAttributeIds::Duration;
		inline static const sas::AttributeId& InitialTargetRange = vega::CommonAttributeIds::Range;
		inline static const sas::AttributeId& SpreadRadius = vega::CommonAttributeIds::Radius;

		// These express Nano Plague policy, so they remain family-owned.
		inline static const sas::AttributeId EnergyPowerTickScale{
			"Ability.Offense.NanoPlague.EnergyPowerTickScale"
		};
		inline static const sas::AttributeId BaseSpreadTargetCount{
			"Ability.Offense.NanoPlague.BaseSpreadTargetCount"
		};
	};

	inline constexpr float DefaultCooldown = 8.f;
	inline constexpr float DefaultDuration = 4.f;
	inline constexpr float DefaultBaseTickDamage = 1.f;
	inline constexpr float DefaultEnergyPowerTickScale = 0.05f;
	inline constexpr float DefaultInitialTargetRange = 800.f;
	inline constexpr float DefaultSpreadRadius = 300.f;
	inline constexpr int DefaultBaseSpreadTargetCount = 1;
}
