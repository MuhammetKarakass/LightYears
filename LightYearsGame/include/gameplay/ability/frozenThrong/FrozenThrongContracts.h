#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::FrozenThrong
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.FrozenThrong.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.FrozenThrong" };
	inline const ly::GameplayTag FamilyTag =
		ly::GameplayTags::Ability::Family::FrozenThrong;

	struct State
	{
		inline static const ly::GameplayTag Active =
			ly::GameplayTags::State::Ability::FrozenThrong::Active;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::FrozenThrong::Started;
		inline static const ly::GameplayTag HuskSpawned =
			ly::GameplayTags::Event::Ability::FrozenThrong::HuskSpawned;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::FrozenThrong::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId HuskDamage =
			ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId LuckToHuskScale{
			"Ability.Offense.FrozenThrong.LuckToHuskScale"
		};
		inline static const sas::AttributeId HuskDelay{
			"Ability.Offense.FrozenThrong.HuskDelay"
		};
		inline static const sas::AttributeId TargetSearchRadius{
			"Ability.Offense.FrozenThrong.TargetSearchRadius"
		};
		inline static const sas::AttributeId DensityRadius{
			"Ability.Offense.FrozenThrong.DensityRadius"
		};
		inline static const sas::AttributeId ExplosionRadius =
			ly::CommonAttributeIds::Radius;
	};

	struct Actor
	{
		struct Husk
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.FrozenThrong.Husk.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.FrozenThrong.Husk"
			};
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.FrozenThrong.Husk.ProjectileSpeed"
			};
		};
	};

	inline constexpr float DefaultDuration = 6.f;
	inline constexpr float DefaultCooldown = 16.f;
	inline constexpr float DefaultHuskDamage = 24.f;
	inline constexpr float DefaultLuckToHuskScale = 0.01f;
	inline constexpr float DefaultHuskDelay = 1.f;
	inline constexpr float DefaultProjectileSpeed = 1100.f;
	inline constexpr float DefaultTargetSearchRadius = 900.f;
	inline constexpr float DefaultDensityRadius = 180.f;
	inline constexpr float DefaultExplosionRadius = 120.f;
	inline constexpr float DefaultExplosionCryoStacks = 2.f;
	inline constexpr float DefaultAttackPowerScale = 0.60f;
}
