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

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.NanoPlague" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::NanoPlague;

	struct State
	{
		inline static const ly::GameplayTag Infected =
			ly::GameplayTags::State::Effect::NanoPlague::Infected;
	};

	struct Attribute
	{
		// Shared output/delivery channels retain the project-wide meanings.
		inline static const sas::AttributeId& BaseTickDamage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId& Duration = ly::CommonAttributeIds::Duration;
		inline static const sas::AttributeId& TickInterval = ly::CommonAttributeIds::Interval;
		inline static const sas::AttributeId& InitialTargetRange = ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId& SpreadRadius = ly::CommonAttributeIds::Radius;

		// These express Nano Plague policy, so they remain family-owned.
		inline static const sas::AttributeId EnergyPowerTickScale{
			"Ability.Offense.NanoPlague.EnergyPowerTickScale"
		};
		inline static const sas::AttributeId BaseSpreadTargetCount{
			"Ability.Offense.NanoPlague.BaseSpreadTargetCount"
		};
		inline static const sas::AttributeId MaximumSpreadTargetCount{
			"Ability.Offense.NanoPlague.MaximumSpreadTargetCount"
		};
	};

	inline constexpr float DefaultCooldown = 8.f;
	inline constexpr float DefaultDuration = 3.f;
	inline constexpr float DefaultTickInterval = 0.25f;
	inline constexpr float DefaultBaseTickDamage = 4.f;
	inline constexpr float DefaultEnergyPowerTickScale = 0.03f;
	inline constexpr float DefaultInitialTargetRange = 800.f;
	inline constexpr float DefaultSpreadRadius = 300.f;
	inline constexpr int DefaultBaseSpreadTargetCount = 1;
	inline constexpr int DefaultMaximumSpreadTargetCount = 5;
}
