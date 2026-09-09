#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/content/NumericSettingContract.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ArcScythes
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.ArcScythes.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Offense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ArcScythes" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::ArcScythes };

	struct State
	{
		// The beam actor consumes this tag, so it cannot outlive its ability's
		// active lifecycle after a cancellation, stun, or normal duration end.
		inline static const ly::GameplayTag Active{ "State.Ability.ArcScythes.Active" };
	};

	struct Setting
	{
		// A policy value rather than an actor stat: it governs when a repeated
		// input may end the active ability, so it stays in JSON settings.
		inline static constexpr char MinActiveDuration[] = "minActiveDuration";

		inline static const ly::content::NumericSettingContract Contract{
			{ MinActiveDuration },
			{ MinActiveDuration }
		};
	};

	struct Actor
	{
		struct Beam
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.ArcScythes.Beam.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.ArcScythes.Beam"
			};
		};
	};

	struct Attribute
	{
		// These are reusable ability outputs rather than Arc-only concepts.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId Range = ly::CommonAttributeIds::Range;

		// The cadence and collision width belong to this continuous-beam delivery
		// model, not to every ranged ability in the project.
		inline static const sas::AttributeId CombatTickInterval{
			"AbilityActor.ArcScythes.Beam.CombatTickInterval"
		};
		inline static const sas::AttributeId BeamHalfThickness{
			"AbilityActor.ArcScythes.Beam.BeamHalfThickness"
		};

		// Electric behavior is already a shared damage-payload contract. Arc
		// Scythes declares values for it; DamageTypeSystem applies the status.
		inline static const sas::AttributeId ElectricStacks =
			ly::DamageAttributeIds::ElectricStacks;
		inline static const sas::AttributeId ElectricDamageTakenMultiplierPerStack =
			ly::DamageAttributeIds::ElectricDamageTakenMultiplierPerStack;
		inline static const sas::AttributeId ElectricDuration =
			ly::DamageAttributeIds::ElectricDuration;
		inline static const sas::AttributeId ElectricMaxStacks =
			ly::DamageAttributeIds::ElectricMaxStacks;
	};
}
