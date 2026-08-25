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

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Movement };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.EnergySpear" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::EnergySpear
	};

	struct State
	{
		inline static const ly::GameplayTag Focusing{
			ly::GameplayTags::State::Ability::EnergySpear::Focusing
		};
		inline static const ly::GameplayTag Traversing{
			ly::GameplayTags::State::Ability::EnergySpear::Traversing
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::EnergySpear::Started
		};
		inline static const ly::GameplayTag MaxDistanceReached{
			ly::GameplayTags::Event::Ability::EnergySpear::MaxDistanceReached
		};
		inline static const ly::GameplayTag Released{
			ly::GameplayTags::Event::Ability::EnergySpear::Released
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::EnergySpear::Ended
		};
	};

	struct Attribute
	{
		// Shared channels are aliases. Energy Spear does not invent duplicate
		// Damage, Range or Collision IDs for concepts already in the schema.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId MaximumDistance = ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId CollisionRadius = ly::CommonAttributeIds::Radius;

		// These values describe Energy Spear's unique charge and movement formula.
		// They are ability attributes, not JSON settings, so they use the normal
		// ability level, attachment and runtime resolution pipeline.
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
		inline static const sas::AttributeId EnergyMaxReference{
			"Ability.Movement.EnergySpear.EnergyMaxReference"
		};
		inline static const sas::AttributeId EnergyMaxDistanceScale{
			"Ability.Movement.EnergySpear.EnergyMaxDistanceScale"
		};
		inline static const sas::AttributeId TravelSpeed{
			"Ability.Movement.EnergySpear.TravelSpeed"
		};
	};
}
