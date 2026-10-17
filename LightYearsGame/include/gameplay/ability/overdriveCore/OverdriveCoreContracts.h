#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::OverdriveCore
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.OverdriveCore.Basic";
	};

	struct Attribute
	{
		// Overdrive intentionally consumes the shared projectile-count contract.
		// This is an alias for discoverability, not a second family-local ID.
		inline static const sas::AttributeId ProjectileCount =
			vega::CommonAttributeIds::ProjectileCount;

		// These values are ability-owned because they describe Overdrive's
		// lifecycle and scaling policy, not a reusable projectile actor property.
		inline static const sas::AttributeId RocketLaunchDuration{
			"Ability.Offense.OverdriveCore.RocketLaunchDuration"
		};
		inline static const sas::AttributeId AttackSpeedBoostDuration{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostDuration"
		};
		inline static const sas::AttributeId AttackSpeedBoostBase{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostBase"
		};
	};

	// The effect duration remains content-owned in effects.json; this ID only
	// gives runtime code a typed reference to the policy effect it applies.
	struct Effect
	{
		inline static constexpr char AttackSpeedBoostId[] =
			"Effect.Offense.OverdriveCore.AttackSpeedBoost.Basic";
	};

	struct Actor
	{
		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.OverdriveCore.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.OverdriveCore.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.OverdriveCore.Projectile.ProjectileSpeed"
			};
		};
	};

	struct State
	{
		inline static const vega::GameplayTag RocketLaunchActive =
			vega::GameplayTags::State::Ability::OverdriveCore::RocketLaunch::Active;
		inline static const vega::GameplayTag AttackSpeedBoostActive =
			vega::GameplayTags::State::Ability::OverdriveCore::AttackSpeedBoost::Active;
	};

	struct Event
	{
		inline static const vega::GameplayTag RocketLaunchStarted =
			vega::GameplayTags::Event::Ability::OverdriveCore::RocketLaunchStarted;
		inline static const vega::GameplayTag RocketLaunchEnded =
			vega::GameplayTags::Event::Ability::OverdriveCore::RocketLaunchEnded;
		inline static const vega::GameplayTag AttackSpeedBoostStarted =
			vega::GameplayTags::Event::Ability::OverdriveCore::AttackSpeedBoostStarted;
		inline static const vega::GameplayTag AttackSpeedBoostEnded =
			vega::GameplayTags::Event::Ability::OverdriveCore::AttackSpeedBoostEnded;
	};

}
