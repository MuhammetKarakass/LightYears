#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::FrostMaelstrom
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Control.FrostMaelstrom.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Control;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.FrostMaelstrom" };
	inline const ly::GameplayTag FamilyTag =
		ly::GameplayTags::Ability::Family::FrostMaelstrom;

	struct State
	{
		inline static const ly::GameplayTag Active =
			ly::GameplayTags::State::Ability::FrostMaelstrom::Active;
		inline static const ly::GameplayTag Controlling =
			ly::GameplayTags::State::Ability::FrostMaelstrom::Controlling;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::FrostMaelstrom::Started;
		inline static const ly::GameplayTag Tick =
			ly::GameplayTags::Event::Ability::FrostMaelstrom::Tick;
		inline static const ly::GameplayTag Released =
			ly::GameplayTags::Event::Ability::FrostMaelstrom::Released;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::FrostMaelstrom::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId MinimumRadius{
			"Ability.Control.FrostMaelstrom.MinimumRadius"
		};
		inline static const sas::AttributeId MaximumRadius{
			"Ability.Control.FrostMaelstrom.MaximumRadius"
		};
		inline static const sas::AttributeId MinimumMovementSpeed{
			"Ability.Control.FrostMaelstrom.MinimumMovementSpeed"
		};
		inline static const sas::AttributeId MaximumMovementSpeed{
			"Ability.Control.FrostMaelstrom.MaximumMovementSpeed"
		};
		inline static const sas::AttributeId TickInterval{
			"Ability.Control.FrostMaelstrom.TickInterval"
		};
		inline static const sas::AttributeId CryoStacksPerTick{
			"Ability.Control.FrostMaelstrom.CryoStacksPerTick"
		};
		inline static const sas::AttributeId OrbitalAngularSpeed{
			"Ability.Control.FrostMaelstrom.OrbitalAngularSpeed"
		};
		inline static const sas::AttributeId InwardForce{
			"Ability.Control.FrostMaelstrom.InwardForce"
		};
		inline static const sas::AttributeId OrbitalRadiusRatio{
			"Ability.Control.FrostMaelstrom.OrbitalRadiusRatio"
		};
		inline static const sas::AttributeId EnergyMaxReference{
			"Ability.Control.FrostMaelstrom.EnergyMaxReference"
		};
		inline static const sas::AttributeId EnergyMaxDamageScale{
			"Ability.Control.FrostMaelstrom.EnergyMaxDamageScale"
		};
		inline static const sas::AttributeId EnergyMaxRadiusScale{
			"Ability.Control.FrostMaelstrom.EnergyMaxRadiusScale"
		};

		// Tick damage uses the project-wide Common.Damage identity so normal
		// AttackPower scaling and damage attachments remain reusable.
		inline static const sas::AttributeId TickDamage =
			ly::CommonAttributeIds::Damage;
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
	inline constexpr float DefaultCooldown = 12.f;
	inline constexpr float DefaultMinimumRadius = 300.f;
	inline constexpr float DefaultMaximumRadius = 600.f;
	inline constexpr float DefaultMinimumMovementSpeed = 200.f;
	inline constexpr float DefaultMaximumMovementSpeed = 500.f;
	inline constexpr float DefaultTickInterval = 0.25f;
	inline constexpr float DefaultCryoStacksPerTick = 1.f;
	// The field must pull targets decisively toward the inner orbit instead of
	// only nudging them.  Angular speed is expressed in radians per second.
	inline constexpr float DefaultOrbitalAngularSpeed = 4.5f;
	inline constexpr float DefaultInwardForce = 1500.f;
	inline constexpr float DefaultOrbitalRadiusRatio = 0.42f;
	inline constexpr float DefaultEnergyMaxReference = 50.f;
	inline constexpr float DefaultEnergyMaxDamageScale = 0.005f;
	inline constexpr float DefaultEnergyMaxRadiusScale = 0.20f;
	inline constexpr float DefaultTickDamage = 2.f;
}
