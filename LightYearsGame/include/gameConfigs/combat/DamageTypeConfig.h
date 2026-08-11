#pragma once

#include "attributes/GameplayAttribute.h"
#include "gameplay/tags/GameplayTags.h"

namespace ly
{
	// Damage type is identity, not a numeric stat. Numeric behavior is expressed
	// by DamagePayload and ordinary source attributes below.
	struct DamageTypeSchema
	{
		inline static const GameplayTag& Root = GameplayTags::Damage::Type::Root;
		inline static const GameplayTag& Photonic = GameplayTags::Damage::Type::Photonic;
		inline static const GameplayTag& Energy = GameplayTags::Damage::Type::Energy;
		inline static const GameplayTag& Kinetic = GameplayTags::Damage::Type::Kinetic;
		inline static const GameplayTag& Thermal = GameplayTags::Damage::Type::Thermal;
		inline static const GameplayTag& Cryo = GameplayTags::Damage::Type::Cryo;
		inline static const GameplayTag& Electric = GameplayTags::Damage::Type::Electric;
	};

	// Source attributes that override or extend a damage type's normal behavior.
	struct DamageAttributeIds
	{
		inline static const sas::AttributeId Root{ "Damage" };
		inline static const sas::AttributeId ShieldDamageMultiplier{ "Damage.ShieldDamageMultiplier" };
		inline static const sas::AttributeId ShieldRegenerationDelay{ "Damage.ShieldRegenerationDelay" };
		inline static const sas::AttributeId ArmorPenetration{ "Damage.ArmorPenetration" };
		inline static const sas::AttributeId IgniteStacks{ "Damage.IgniteStacks" };
		inline static const sas::AttributeId BurnDamagePerSecond{ "Damage.BurnDamagePerSecond" };
		inline static const sas::AttributeId BurnDuration{ "Damage.BurnDuration" };
		inline static const sas::AttributeId BurnMaxStacks{ "Damage.BurnMaxStacks" };
		inline static const sas::AttributeId CryoBuildupPerHit{ "Damage.Cryo.BuildupPerHit" };
		inline static const sas::AttributeId CryoBuildupRequired{ "Damage.Cryo.BuildupRequired" };
		inline static const sas::AttributeId CryoBuildupDuration{ "Damage.Cryo.BuildupDuration" };
		inline static const sas::AttributeId CryoSlowPercent{ "Damage.Cryo.SlowPercent" };
		inline static const sas::AttributeId CryoSlowDuration{ "Damage.Cryo.SlowDuration" };
		inline static const sas::AttributeId ElectricStacks{ "Damage.ElectricStacks" };
		inline static const sas::AttributeId ElectricDamageTakenMultiplierPerStack{
			"Damage.ElectricDamageTakenMultiplierPerStack"
		};
		inline static const sas::AttributeId ElectricDuration{ "Damage.ElectricDuration" };
		inline static const sas::AttributeId ElectricMaxStacks{ "Damage.ElectricMaxStacks" };
	};

	struct DamageStatusSchema
	{
		inline static const GameplayTag& Ignite = GameplayTags::Status::Damage::Ignite;
		inline static const GameplayTag& CryoBuildup = GameplayTags::Status::Damage::Cryo::Buildup;
		inline static const GameplayTag& CryoSlowed = GameplayTags::Status::Damage::Cryo::Slowed;
		inline static const GameplayTag& Electric = GameplayTags::Status::Damage::Electric;
	};

	struct DamageStatusEffectIds
	{
		inline static constexpr char IgniteEffectId[] =
			"Effect.Status.Damage.Ignite";
		inline static constexpr char CryoBuildupEffectId[] =
			"Effect.Status.Damage.Cryo.Buildup";
		inline static constexpr char CryoSlowedEffectId[] =
			"Effect.Status.Damage.Cryo.Slowed";
		inline static constexpr char ElectricEffectId[] =
			"Effect.Status.Damage.Electric";
	};
}
