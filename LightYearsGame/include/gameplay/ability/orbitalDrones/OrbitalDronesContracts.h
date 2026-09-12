#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::OrbitalDrones
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.OrbitalDrones.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.OrbitalDrones" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::OrbitalDrones
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::OrbitalDrones::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::OrbitalDrones::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::OrbitalDrones::Ended
		};
	};

	struct Attribute
	{
		// Damage and orbit radius are shared numeric concepts. Reusing Common.*
		// keeps this family compatible with the generic ability attribute pipeline.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;

		// These values describe drone-specific rules, so they are owned by this
		// ability family instead of being placed in a generic Damage.* namespace.
		inline static const sas::AttributeId DroneCount{
			"Ability.Offense.OrbitalDrones.DroneCount"
		};
		inline static const sas::AttributeId SameTargetHitCooldown{
			"Ability.Offense.OrbitalDrones.SameTargetHitCooldown"
		};
		// The base orbital velocity is family-owned because it describes the
		// drone formation. It is used directly unless a future explicit scaling
		// rule is added to this ability family.
		inline static const sas::AttributeId BaseAngularSpeedRadiansPerSecond{
			"Ability.Offense.OrbitalDrones.BaseAngularSpeedRadiansPerSecond"
		};
		// Contact radius belongs to this ability's damage rule. It must not use
		// Collision.Radius here because ability-level attributes may only be
		// Common.* or family-owned identifiers.
		inline static const sas::AttributeId ContactRadius{
			"Ability.Offense.OrbitalDrones.ContactRadius"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Offense.OrbitalDrones.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerDurationScale{
			"Ability.Offense.OrbitalDrones.EnergyPowerDurationScale"
		};

	};
}
