#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::EnergySpear
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.EnergySpear.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Movement };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::EnergySpear
	};

	struct State
	{
		inline static const vega::GameplayTag Focusing{
			vega::GameplayTags::State::Ability::EnergySpear::Focusing
		};
		inline static const vega::GameplayTag Traversing{
			vega::GameplayTags::State::Ability::EnergySpear::Traversing
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::EnergySpear::Started
		};
		inline static const vega::GameplayTag MaxDistanceReached{
			vega::GameplayTags::Event::Ability::EnergySpear::MaxDistanceReached
		};
		inline static const vega::GameplayTag Released{
			vega::GameplayTags::Event::Ability::EnergySpear::Released
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::EnergySpear::Ended
		};
	};

	struct Attribute
	{
		// Shared channels are aliases. Energy Spear does not invent duplicate
		// Damage, Range or Collision IDs for concepts already in the schema.
		inline static const sas::AttributeId Damage = vega::CommonAttributeIds::Damage;
		inline static const sas::AttributeId MaximumDistance = vega::CommonAttributeIds::Range;
		inline static const sas::AttributeId CollisionRadius = vega::CommonAttributeIds::Radius;

		// These values describe Energy Spear's unique charge and movement formula.
		// They are ability attributes, not JSON settings, so they use the normal
		// ability level and runtime resolution pipeline.
		inline static const sas::AttributeId MinimumDistance{
			"Ability.Movement.EnergySpear.MinimumDistance"
		};
		inline static const sas::AttributeId MaximumDistanceChargeThreshold{
			"Ability.Movement.EnergySpear.MaximumDistanceChargeThreshold"
		};
		inline static const sas::AttributeId ChargeDamageMultiplierAtFull{
			"Ability.Movement.EnergySpear.ChargeDamageMultiplierAtFull"
		};
		inline static const sas::AttributeId DistanceDamageMultiplierAtEndpoint{
			"Ability.Movement.EnergySpear.DistanceDamageMultiplierAtEndpoint"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Movement.EnergySpear.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerDistanceScale{
			"Ability.Movement.EnergySpear.EnergyPowerDistanceScale"
		};
		inline static const sas::AttributeId TravelSpeed{
			"Ability.Movement.EnergySpear.TravelSpeed"
		};
	};
}
