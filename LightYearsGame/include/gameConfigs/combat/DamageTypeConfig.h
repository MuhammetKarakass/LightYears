#pragma once

#include "attributes/GameplayAttribute.h"
#include "gameplay/tags/GameplayTags.h"

#include <array>
#include <algorithm>

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

	// Source attributes that override or extend a damage type's delivery behavior.
	struct DamageAttributeIds
	{
		inline static const sas::AttributeId Root{ "Damage" };
		inline static const sas::AttributeId ShieldDamageMultiplier{ "Damage.ShieldDamageMultiplier" };
		inline static const sas::AttributeId ShieldRegenerationDelay{ "Damage.ShieldRegenerationDelay" };
		inline static const sas::AttributeId ArmorPenetration{ "Damage.ArmorPenetration" };
		inline static const sas::AttributeId KineticStacks{ "Damage.Kinetic.Stacks" };
		inline static const sas::AttributeId IgniteStacks{ "Damage.IgniteStacks" };
		inline static const sas::AttributeId BurnDamagePerTick{ "Damage.BurnDamagePerTick" };
		inline static const sas::AttributeId BurnTickInterval{ "Damage.BurnTickInterval" };
		inline static const sas::AttributeId BurnTickAccumulator{ "Damage.BurnTickAccumulator" };
		inline static const sas::AttributeId BurnDuration{ "Damage.BurnDuration" };
		inline static const sas::AttributeId CryoBuildupPerHit{ "Damage.Cryo.BuildupPerHit" };
		inline static const sas::AttributeId ElectricStacks{ "Damage.ElectricStacks" };
	};

	struct DamageStatusFamilyBalance
	{
		inline static constexpr std::size_t ValueCount = 4;
		float duration = 0.f;
		int maxStacks = 0;
		std::array<float, ValueCount> values{};

		float ValueForStack(int stack) const
		{
			if (stack <= 0 || maxStacks < 1)
			{
				return 0.f;
			}
			const int safeMaxStacks = std::clamp(
				maxStacks,
				1,
				static_cast<int>(values.size())
			);
			return values[std::clamp(stack, 1, safeMaxStacks) - 1];
		}
	};

	struct DamageStatusBalance
	{
		float energyShieldDamageMultiplier = 0.f;
		DamageStatusFamilyBalance cryo;
		DamageStatusFamilyBalance electric;
		DamageStatusFamilyBalance thermal;
		DamageStatusFamilyBalance kinetic;

		float CryoSlowPercent(int stack) const
		{
			return cryo.ValueForStack(stack);
		}
		float ElectricDamageTakenMultiplier(int stack) const
		{
			return electric.ValueForStack(stack);
		}
		float ThermalDamagePerSecond(int stack) const
		{
			return thermal.ValueForStack(stack);
		}
		float KineticArmorPenetration(int stack) const
		{
			return kinetic.ValueForStack(stack);
		}
	};

	struct DamageStatusSchema
	{
		inline static const GameplayTag& Ignite = GameplayTags::Status::Damage::Ignite;
		inline static const GameplayTag& CryoSlowed = GameplayTags::Status::Damage::Cryo::Slowed;
		inline static const GameplayTag& Electric = GameplayTags::Status::Damage::Electric;
		inline static const GameplayTag& Kinetic = GameplayTags::Status::Damage::Kinetic;
	};

	struct DamageStatusEffectIds
	{
		inline static constexpr char IgniteEffectId[] =
			"Effect.Status.Damage.Ignite";
		inline static constexpr char CryoSlowedEffectId[] =
			"Effect.Status.Damage.Cryo.Slowed";
		inline static constexpr char ElectricEffectId[] =
			"Effect.Status.Damage.Electric";
		inline static constexpr char KineticEffectId[] =
			"Effect.Status.Damage.Kinetic";
	};
}
