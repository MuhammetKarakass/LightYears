#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::NullPulse
{
	struct AbilityId
	{
		// Ability IDs must include the category, family and variant segments.
		inline static constexpr char Basic[] = "Ability.Utility.NullPulse.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Utility };
	inline const vega::GameplayTag FamilyTag{ vega::GameplayTags::Ability::Family::NullPulse };

	struct Attribute
	{
		// Radius and damage are shared numeric concepts. They are aliases here so
		// the behavior remains readable without creating duplicate AttributeIds.
		inline static const sas::AttributeId Radius = vega::CommonAttributeIds::Radius;
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;

		// Stun tuning is specific to Null Pulse's control formula and therefore
		// belongs to the ability-scoped attribute list in abilities.json.
		inline static const sas::AttributeId BaseStunDuration{
			"Ability.Utility.NullPulse.BaseStunDuration"
		};
		inline static const sas::AttributeId EnergyPowerDamageScale{
			"Ability.Utility.NullPulse.EnergyPowerDamageScale"
		};
	};

	inline constexpr float EnergyPowerStunDurationScale = 0.01f;

	struct Effect
	{
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	struct Event
	{
		inline static const vega::GameplayTag Activated =
			vega::GameplayTags::Event::Ability::NullPulse::Activated;
		inline static const vega::GameplayTag ProjectilesCleared =
			vega::GameplayTags::Event::Ability::NullPulse::ProjectilesCleared;
		inline static const vega::GameplayTag Completed =
			vega::GameplayTags::Event::Ability::NullPulse::Completed;
	};
}
