#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/combat/WeaponStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

// Readable stat helpers keep the gameplay tags out of weapon progression data.
namespace WeaponData::WeaponGrowth
{
	inline sas::AttributeModifier Add(const ly::GameplayTag& attributeId, float value)
	{
		return { attributeId, sas::AttributeModifierOperation::Add, value };
	}

	inline sas::AttributeModifier Damage(float value)
	{
		return Add(ly::CommonAttributeIds::Damage, value);
	}

	inline sas::AttributeModifier FireRate(float value)
	{
		return Add(ly::CommonAttributeIds::FireRate, value);
	}

	inline sas::AttributeModifier Range(float value)
	{
		return Add(ly::CommonAttributeIds::Range, value);
	}

	inline sas::AttributeModifier ProjectileSpeed(float value)
	{
		return Add(PrimaryWeaponSchema::Projectile::Delivery::Speed, value);
	}

	inline sas::AttributeModifier ProjectilePierce(float value)
	{
		return Add(PrimaryWeaponSchema::Projectile::Delivery::PierceCount, value);
	}

	inline sas::AttributeModifier ChainCount(float value)
	{
		return Add(PrimaryWeaponSchema::Arc::Electric::ChainCount, value);
	}

	inline sas::AttributeModifier ChainRange(float value)
	{
		return Add(PrimaryWeaponSchema::Arc::Electric::ChainRange, value);
	}

	inline sas::AttributeModifier ChainDamageMultiplier(float value)
	{
		return Add(PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain, value);
	}

	inline sas::AttributeModifier BeamRange(float value)
	{
		return Add(PrimaryWeaponSchema::Beam::Delivery::Range, value);
	}

	inline sas::AttributeModifier HeatDamageAtMax(float value)
	{
		return Add(PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat, value);
	}

	inline sas::AttributeModifier WaveMaximumWidth(float value)
	{
		return Add(PrimaryWeaponSchema::Wave::Delivery::MaximumWidth, value);
	}

	inline sas::AttributeModifier CryoBuildupDuration(float value)
	{
		return Add(ly::DamageAttributeIds::CryoBuildupDuration, value);
	}
}

// Profiles are intentionally separate from base weapon definitions. This makes
// balancing level growth readable while retaining flexible per-level milestones.
namespace WeaponData::WeaponProgressions
{
	using namespace WeaponGrowth;

	static const WeaponProgressionProfile BasicRapidLaser = WeaponProgressionProfile{ 4 }
		.ScrapCosts({ 40u, 50u, 65u })
		.AtLevel(2, { FireRate(1.f) })
		.AtLevel(3, { Damage(2.f), ProjectileSpeed(100.f) })
		.AtLevel(4, { Damage(2.f), FireRate(1.f), Range(100.f) });

	static const WeaponProgressionProfile DualKineticBlaster = WeaponProgressionProfile{ 4 }
		.ScrapCosts({ 40u, 50u, 65u })
		.AtLevel(2, { FireRate(1.5f) })
		.AtLevel(3, { Damage(1.f) })
		.AtLevel(4, { Range(100.f), FireRate(1.f) });

	static const WeaponProgressionProfile ElectricArcLauncher = WeaponProgressionProfile{ 4 }
		.ScrapCosts({ 40u, 50u, 65u })
		.AtLevel(2, { Damage(4.f) })
		.AtLevel(3, { ChainCount(1.f), ChainRange(80.f) })
		.AtLevel(4, { FireRate(0.4f), ChainDamageMultiplier(0.08f) });

	static const WeaponProgressionProfile ContinuousHeatLaser = WeaponProgressionProfile{ 4 }
		.ScrapCosts({ 40u, 50u, 65u })
		.AtLevel(2, { Damage(6.f) })
		.AtLevel(3, { BeamRange(150.f) })
		.AtLevel(4, { HeatDamageAtMax(0.2f) });

	static const WeaponProgressionProfile CryoWaveProjector = WeaponProgressionProfile{ 4 }
		.ScrapCosts({ 40u, 50u, 65u })
		.AtLevel(2, { Damage(2.f), WaveMaximumWidth(30.f) })
		.AtLevel(3, { Range(100.f), CryoBuildupDuration(0.5f) })
		.AtLevel(4, { FireRate(0.3f), Damage(2.f) });
}
