#pragma once

#include "gameConfigs/presentation/PointLightConfig.h"
#include "gameConfigs/combat/WeaponProgressionConfig.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/damage/DamageTypeSystem.h"

namespace WeaponData
{
	static const WeaponPresentationDefinition Laser_Blue_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
		LightingData::Laser_Blue_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const WeaponPresentationDefinition Laser_Red_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserRed01.png",
		LightingData::Laser_Red_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const WeaponPresentationDefinition Laser_Green_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserGreen11.png",
		LightingData::Laser_Green_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const WeaponPresentationDefinition Kinetic_Red_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserRed04.png",
		LightingData::Laser_Red_PointLightDef,
		{ 0.f, 30.f },
		0.9f
	);

	static const WeaponPresentationDefinition Electric_Blue_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserBlue16.png",
		LightingData::Laser_Blue_PointLightDef,
		{ 0.f, 32.f },
		1.1f
	);

	static const WeaponPresentationDefinition Cryo_Wave_PresentationDef(
		"",
		LightingData::Laser_Blue_PointLightDef,
		{ 0.f, 0.f },
		1.f
	);
}

namespace WeaponData::PrimaryWeapons
{
	static const PrimaryWeaponDefinition BasicRapidLaser(
		"FighterBasicRapidLaser",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Blue_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 8.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 8.f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 1100.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 0, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1600.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 7.f, 0.1f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::AreaRadius, 0.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		WeaponProgressions::BasicRapidLaser,
		{},
		{
			ly::AttributeScalingRule{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, ly::AttributeModifierOperation::Add, 1.f },
			ly::AttributeScalingRule{ ly::CommonAttributeIds::FireRate, ly::OwnerAttributeIds::AttackSpeed, ly::AttributeModifierOperation::Add, 1.f }
		},
		{},
		{},
		{ ly::DamageTypeSchema::Photonic }
	);

	static const PrimaryWeaponDefinition RapidShotgun(
		"PlayerRapidShotgun",
		PrimaryWeaponSchema::Projectile::Shotgun::TypeId,
		Laser_Green_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 2.5f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 3000.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 0.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 400.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 7.f, 0.1f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::AreaRadius, 0.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 3.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle, 8.f, 0.f },
			ly::GameplayAttribute{
				PrimaryWeaponSchema::Projectile::Shotgun::DamageReductionPerAdditionalHit,
				0.1f,
				0.f
			},
			ly::GameplayAttribute{
				PrimaryWeaponSchema::Projectile::Shotgun::MinimumDamageMultiplier,
				0.5f,
				0.01f
			}
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		{},
		{},
		{
			ly::AttributeScalingRule{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, ly::AttributeModifierOperation::Add, 0.75f },
			ly::AttributeScalingRule{ ly::CommonAttributeIds::FireRate, ly::OwnerAttributeIds::AttackSpeed, ly::AttributeModifierOperation::Add, 0.5f }
		},
		{},
		{},
		{ ly::DamageTypeSchema::Thermal }
	);

	static const PrimaryWeaponDefinition DualKineticBlaster(
		"PlayerDualKineticBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Kinetic_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 3.5f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 12.f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 3400.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 0.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 650.f, 1.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 6.f, 0.1f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::AreaRadius, 0.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -18.f, 48.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 18.f, 48.f }, 0.f }
		},
		true,
		WeaponProgressions::DualKineticBlaster,
		{},
		{
			ly::AttributeScalingRule{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, ly::AttributeModifierOperation::Add, 0.45f },
			ly::AttributeScalingRule{ ly::CommonAttributeIds::FireRate, ly::OwnerAttributeIds::AttackSpeed, ly::AttributeModifierOperation::Add, 1.f }
		},
		{},
		{},
		{ ly::DamageTypeSchema::Kinetic }
	);

	static const PrimaryWeaponDefinition ElectricArcLauncher(
		"PlayerElectricArcLauncher",
		PrimaryWeaponSchema::Arc::Electric::TypeId,
		Electric_Blue_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 15.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 2.8f, 0.01f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 850.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainCount, 3.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainRange, 250.f, 1.f },
			ly::GameplayAttribute{
				PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain,
				0.72f,
				0.01f,
				1.f
			}
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		WeaponProgressions::ElectricArcLauncher,
		{},
		{
			ly::AttributeScalingRule{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, ly::AttributeModifierOperation::Add, 0.85f },
			ly::AttributeScalingRule{ ly::CommonAttributeIds::FireRate, ly::OwnerAttributeIds::AttackSpeed, ly::AttributeModifierOperation::Add, 0.60f }
		},
		{},
		{},
		{ ly::DamageTypeSchema::Electric }
	);

	// Damage is damage per second for this weapon type, not damage per fired projectile.
	static const PrimaryWeaponDefinition ContinuousHeatLaser(
		"PlayerContinuousHeatLaser",
		PrimaryWeaponSchema::Beam::Continuous::TypeId,
		Laser_Blue_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 28.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Range, 950.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 26.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Gain, 38.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Dissipation, 25.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::OverheatCooldown, 2.5f, 0.1f },
			ly::GameplayAttribute{
				PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat,
				1.75f,
				1.f
			}
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		WeaponProgressions::ContinuousHeatLaser,
		{},
		{
			ly::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				ly::AttributeModifierOperation::Add,
				0.75f
			},
			ly::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyMax,
				ly::AttributeModifierOperation::Add,
				0.50f
			}
		},
		{ PrimaryWeaponSchema::Feature::Heat::FeatureId },
		{
			HeatGainCurveSegmentDefinition{ 50.f, 1.f },
			HeatGainCurveSegmentDefinition{ 75.f, 0.5f },
			HeatGainCurveSegmentDefinition{ 90.f, 0.25f },
			HeatGainCurveSegmentDefinition{ 100.f, 0.15f }
		},
		{ ly::DamageTypeSchema::Energy }
	);

	static const PrimaryWeaponDefinition CryoWaveProjector(
		"PlayerCryoWaveProjector",
		PrimaryWeaponSchema::Wave::Expanding::TypeId,
		Cryo_Wave_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 7.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.8f, 0.01f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 780.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Wave::Delivery::Speed, 850.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Wave::Delivery::InitialWidth, 80.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Wave::Delivery::MaximumWidth, 260.f, 1.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Wave::Delivery::Thickness, 30.f, 1.f },
			ly::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupPerHit, 1.f, 1.f },
			ly::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupRequired, 4.f, 1.f },
			ly::GameplayAttribute{ ly::DamageAttributeIds::CryoBuildupDuration, 2.5f, 0.f },
			ly::GameplayAttribute{ ly::DamageAttributeIds::CryoSlowPercent, 0.25f, 0.f, 0.30f },
			ly::GameplayAttribute{ ly::DamageAttributeIds::CryoSlowDuration, 1.5f, 0.f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		WeaponProgressions::CryoWaveProjector,
		{},
		{
			ly::AttributeScalingRule{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, ly::AttributeModifierOperation::Add, 0.75f },
			ly::AttributeScalingRule{ ly::CommonAttributeIds::FireRate, ly::OwnerAttributeIds::AttackSpeed, ly::AttributeModifierOperation::Add, 0.5f }
		},
		{},
		{},
		{ ly::DamageTypeSchema::Cryo }
	);

	static const PrimaryWeaponDefinition VanguardBlaster(
		"EnemyVanguardBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.1f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 40.f }, 0.f } }
	);

	static const PrimaryWeaponDefinition VanguardEliteBlaster(
		"EnemyVanguardEliteBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.35f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 40.f }, 0.f } }
	);

	static const PrimaryWeaponDefinition TwinBladeDualBlaster(
		"EnemyTwinBladeDualBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 400.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -20.f, 40.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 20.f, 40.f }, 0.f }
		}
	);

	static const PrimaryWeaponDefinition HexagonRadialBlaster(
		"EnemyHexagonRadialBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.35f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 400.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, -50.f }, 180.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 50.f, 50.f }, 45.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -50.f, 50.f }, -45.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -50.f, -50.f }, -135.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 50.f, -50.f }, 135.f }
		}
	);

	static const PrimaryWeaponDefinition UfoTriBlaster(
		"EnemyUFOTriBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 1.1f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ 35.f, 20.f }, 60.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -35.f, 20.f }, -60.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, -40.f }, 180.f }
		}
	);

	static const PrimaryWeaponDefinition BossBaseDualBlaster(
		"BossBaseDualBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 2.f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -50.f, 50.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 50.f, 50.f }, 0.f }
		}
	);

	static const PrimaryWeaponDefinition BossThreeWayBlaster(
		"BossThreeWayBlaster",
		PrimaryWeaponSchema::Projectile::Shotgun::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 0.5f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle, 60.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 3.f, 1.f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 100.f }, 0.f } }
	);

	static const PrimaryWeaponDefinition BossFrontalSweep(
		"BossFrontalSweep",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 0.33f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -80.f, 65.f }, 4.5f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -86.7f, 75.f }, 3.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -113.3f, 75.f }, -3.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -120.f, 65.f }, -4.5f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 120.f, 65.f }, 4.5f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 113.3f, 75.f }, 3.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 86.7f, 75.f }, -3.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 80.f, 65.f }, -4.5f }
		}
	);

	static const PrimaryWeaponDefinition BossLastStageSideBlaster(
		"BossLastStageSideBlaster",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Red_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 2.f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.6f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 1900.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f }
		},
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -150.f, 50.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 150.f, 50.f }, 0.f }
		}
	);
}
