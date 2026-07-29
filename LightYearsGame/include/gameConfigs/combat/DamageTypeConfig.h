#pragma once

#include "gameplay/attributes/GameplayAttribute.h"

namespace ly
{
	// Damage type is identity, not a numeric stat. Numeric behavior is expressed
	// by DamagePayload and ordinary source attributes below.
	struct DamageTypeSchema
	{
		inline static const GameplayTag Root{ "Damage.Type" };
		inline static const GameplayTag Photonic{ "Damage.Type.Photonic" };
		inline static const GameplayTag Energy{ "Damage.Type.Energy" };
		inline static const GameplayTag Kinetic{ "Damage.Type.Kinetic" };
		inline static const GameplayTag Thermal{ "Damage.Type.Thermal" };
		inline static const GameplayTag Cryo{ "Damage.Type.Cryo" };
		inline static const GameplayTag Electric{ "Damage.Type.Electric" };
	};

	// Source attributes that override or extend a damage type's normal behavior.
	struct DamageAttributeIds
	{
		inline static const GameplayTag AttributeRoot{ "Attribute.Damage" };
		inline static const GameplayTag ShieldDamageMultiplier{ "Attribute.Damage.ShieldDamageMultiplier" };
		inline static const GameplayTag ShieldRegenerationDelay{ "Attribute.Damage.ShieldRegenerationDelay" };
		inline static const GameplayTag ArmorPenetration{ "Attribute.Damage.ArmorPenetration" };
		inline static const GameplayTag IgniteStacks{ "Attribute.Damage.IgniteStacks" };
		inline static const GameplayTag BurnDamagePerSecond{ "Attribute.Damage.BurnDamagePerSecond" };
		inline static const GameplayTag BurnDuration{ "Attribute.Damage.BurnDuration" };
		inline static const GameplayTag BurnMaxStacks{ "Attribute.Damage.BurnMaxStacks" };
		inline static const GameplayTag CryoBuildupPerHit{ "Attribute.Damage.Cryo.BuildupPerHit" };
		inline static const GameplayTag CryoBuildupRequired{ "Attribute.Damage.Cryo.BuildupRequired" };
		inline static const GameplayTag CryoBuildupDuration{ "Attribute.Damage.Cryo.BuildupDuration" };
		inline static const GameplayTag CryoSlowPercent{ "Attribute.Damage.Cryo.SlowPercent" };
		inline static const GameplayTag CryoSlowDuration{ "Attribute.Damage.Cryo.SlowDuration" };
		inline static const GameplayTag ElectricStacks{ "Attribute.Damage.ElectricStacks" };
		inline static const GameplayTag ElectricDamageTakenMultiplierPerStack{
			"Attribute.Damage.ElectricDamageTakenMultiplierPerStack"
		};
		inline static const GameplayTag ElectricDuration{ "Attribute.Damage.ElectricDuration" };
		inline static const GameplayTag ElectricMaxStacks{ "Attribute.Damage.ElectricMaxStacks" };
	};

	struct DamageStatusSchema
	{
		inline static const GameplayTag Ignite{ "Status.Damage.Ignite" };
		inline static const GameplayTag CryoBuildup{ "Status.Damage.Cryo.Buildup" };
		inline static const GameplayTag CryoSlowed{ "Status.Damage.Cryo.Slowed" };
		inline static const GameplayTag Electric{ "Status.Damage.Electric" };
		inline static const GameplayTag IgniteBehavior{ "EffectBehavior.Damage.Ignite" };
		inline static const GameplayTag ElectricBehavior{ "EffectBehavior.Damage.Electric" };
	};

	struct DamageStatusEffectIds
	{
		inline static constexpr const char* CryoBuildup =
			"Effect.Status.Damage.Cryo.Buildup";
		inline static constexpr const char* CryoSlowed =
			"Effect.Status.Damage.Cryo.Slowed";
	};
}
