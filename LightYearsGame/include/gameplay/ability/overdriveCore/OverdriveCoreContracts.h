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
			ly::CommonAttributeIds::ProjectileCount;

		// These values are ability-owned because they describe Overdrive's
		// lifecycle and scaling policy, not a reusable projectile actor property.
		inline static const sas::AttributeId RocketLaunchDuration{
			"Ability.Offense.OverdriveCore.RocketLaunchDuration"
		};
		inline static const sas::AttributeId SameTargetDamageDecay{
			"Ability.Offense.OverdriveCore.SameTargetDamageDecay"
		};
		inline static const sas::AttributeId AttackSpeedBoostDuration{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostDuration"
		};
		inline static const sas::AttributeId AttackSpeedBoostBase{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostBase"
		};
		inline static const sas::AttributeId AttackSpeedBoostPerLevel{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostPerLevel"
		};
		inline static const sas::AttributeId AttackSpeedBoostCriticalChanceScale{
			"Ability.Offense.OverdriveCore.AttackSpeedBoostCriticalChanceScale"
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
		inline static const ly::GameplayTag RocketLaunchActive =
			ly::GameplayTags::State::Ability::OverdriveCore::RocketLaunch::Active;
		inline static const ly::GameplayTag AttackSpeedBoostActive =
			ly::GameplayTags::State::Ability::OverdriveCore::AttackSpeedBoost::Active;
	};

	struct Event
	{
		inline static const ly::GameplayTag RocketLaunchStarted =
			ly::GameplayTags::Event::Ability::OverdriveCore::RocketLaunchStarted;
		inline static const ly::GameplayTag RocketLaunchEnded =
			ly::GameplayTags::Event::Ability::OverdriveCore::RocketLaunchEnded;
		inline static const ly::GameplayTag AttackSpeedBoostStarted =
			ly::GameplayTags::Event::Ability::OverdriveCore::AttackSpeedBoostStarted;
		inline static const ly::GameplayTag AttackSpeedBoostEnded =
			ly::GameplayTags::Event::Ability::OverdriveCore::AttackSpeedBoostEnded;
	};

}
