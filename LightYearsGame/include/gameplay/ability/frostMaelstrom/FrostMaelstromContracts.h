#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::FrostMaelstrom
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Utility.FrostMaelstrom.Basic";
	};

	inline const vega::GameplayTag CategoryTag = vega::GameplayTags::Ability::Utility;
	inline const vega::GameplayTag FamilyTag =
		vega::GameplayTags::Ability::Family::FrostMaelstrom;

	struct State
	{
		inline static const vega::GameplayTag Active =
			vega::GameplayTags::State::Ability::FrostMaelstrom::Active;
		inline static const vega::GameplayTag Controlling =
			vega::GameplayTags::State::Ability::FrostMaelstrom::Controlling;
	};

	struct Event
	{
		inline static const vega::GameplayTag Started =
			vega::GameplayTags::Event::Ability::FrostMaelstrom::Started;
		inline static const vega::GameplayTag Tick =
			vega::GameplayTags::Event::Ability::FrostMaelstrom::Tick;
		inline static const vega::GameplayTag Released =
			vega::GameplayTags::Event::Ability::FrostMaelstrom::Released;
		inline static const vega::GameplayTag Ended =
			vega::GameplayTags::Event::Ability::FrostMaelstrom::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId MinimumRadius{
			"Ability.Utility.FrostMaelstrom.MinimumRadius"
		};
		inline static const sas::AttributeId MaximumRadius{
			"Ability.Utility.FrostMaelstrom.MaximumRadius"
		};
		inline static const sas::AttributeId MinimumMovementSpeed{
			"Ability.Utility.FrostMaelstrom.MinimumMovementSpeed"
		};
		inline static const sas::AttributeId MaximumMovementSpeed{
			"Ability.Utility.FrostMaelstrom.MaximumMovementSpeed"
		};
		inline static const sas::AttributeId CryoStacksPerTick{
			"Ability.Utility.FrostMaelstrom.CryoStacksPerTick"
		};
		inline static const sas::AttributeId OrbitalAngularSpeed{
			"Ability.Utility.FrostMaelstrom.OrbitalAngularSpeed"
		};
		inline static const sas::AttributeId PullStrength{
			"Ability.Utility.FrostMaelstrom.PullStrength"
		};
		inline static const sas::AttributeId OrbitalRadiusRatio{
			"Ability.Utility.FrostMaelstrom.OrbitalRadiusRatio"
		};
		inline static const sas::AttributeId EnergyPowerDamageScale{
			"Ability.Utility.FrostMaelstrom.EnergyPowerDamageScale"
		};

		// Tick damage uses the project-wide Common.Damage identity so normal
		// AttackPower scaling remains reusable.
		inline static const sas::AttributeId TickDamage =
			vega::CommonAttributeIds::Damage;
	};

	struct Actor
	{
		struct Field
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.FrostMaelstrom.Field.Basic";
		};
	};

	inline constexpr float DefaultDuration = 6.f;
	inline constexpr float DefaultCooldown = 14.f;
	inline constexpr float DefaultMinimumRadius = 300.f;
	inline constexpr float DefaultMaximumRadius = 600.f;
	inline constexpr float DefaultMinimumMovementSpeed = 200.f;
	inline constexpr float DefaultMaximumMovementSpeed = 500.f;
	inline constexpr float DefaultCryoStacksPerTick = 1.f;
	// The field must pull targets decisively toward the inner orbit instead of
	// only nudging them.  Angular speed is expressed in radians per second.
	inline constexpr float DefaultOrbitalAngularSpeed = 4.5f;
	inline constexpr float DefaultPullStrength = 300.f;
	inline constexpr float DefaultOrbitalRadiusRatio = 0.42f;
	inline constexpr float DefaultEnergyPowerDamageScale = 0.1f;
	inline constexpr float DefaultTickDamage = 1.f;
}
