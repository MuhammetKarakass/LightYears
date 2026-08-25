#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::CombatSentry
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.CombatSentry.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.CombatSentry" };
	inline const ly::GameplayTag FamilyTag =
		ly::GameplayTags::Ability::Family::CombatSentry;

	struct Event
	{
		inline static const ly::GameplayTag Spawned =
			ly::GameplayTags::Event::Ability::CombatSentry::Spawned;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::CombatSentry::Ended;
	};

	struct Actor
	{
		struct Turret
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.CombatSentry.Turret.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.CombatSentry.Turret"
			};
			inline static const sas::AttributeId MaxHealth{
				"AbilityActor.CombatSentry.Turret.MaxHealth"
			};
			inline static const sas::AttributeId OwnerMaxHealthScale{
				"AbilityActor.CombatSentry.Turret.OwnerMaxHealthScale"
			};
			inline static const sas::AttributeId Armor{
				"AbilityActor.CombatSentry.Turret.Armor"
			};
			inline static const sas::AttributeId OwnerArmorScale{
				"AbilityActor.CombatSentry.Turret.OwnerArmorScale"
			};
			inline static const sas::AttributeId BaseDamage{
				"AbilityActor.CombatSentry.Turret.BaseDamage"
			};
			inline static const sas::AttributeId TargetingRange{
				"AbilityActor.CombatSentry.Turret.TargetingRange"
			};
			inline static const sas::AttributeId AttackRate{
				"AbilityActor.CombatSentry.Turret.AttackRate"
			};
			inline static const sas::AttributeId OwnerAttackPowerScale{
				"AbilityActor.CombatSentry.Turret.OwnerAttackPowerScale"
			};
		};

		struct Projectile
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.CombatSentry.Projectile.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.CombatSentry.Projectile"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.CombatSentry.Projectile.ProjectileSpeed"
			};
		};
	};

	inline constexpr float DefaultDuration = 20.f;
	inline constexpr float DefaultCooldown = 25.f;
	inline constexpr float DefaultMaxHealth = 150.f;
	inline constexpr float DefaultOwnerMaxHealthScale = 0.50f;
	inline constexpr float DefaultArmor = 5.f;
	inline constexpr float DefaultOwnerArmorScale = 0.50f;
	inline constexpr float DefaultDamage = 14.f;
	inline constexpr float DefaultOwnerAttackPowerScale = 0.60f;
	inline constexpr float DefaultAttackRate = 1.25f;
	inline constexpr float DefaultTargetingRange = 800.f;
	inline constexpr float DefaultProjectileRange = 800.f;
	inline constexpr float DefaultProjectileSpeed = 1300.f;
}
