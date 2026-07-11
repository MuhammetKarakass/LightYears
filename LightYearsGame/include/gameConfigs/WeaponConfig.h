#pragma once

#include "gameConfigs/PointLightConfig.h"
#include "gameConfigs/WeaponStructs.h"

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

	static const PrimaryWeaponDefinition Fighter_PrimaryWeaponDef(
		"FighterPulseCannon",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		Laser_Blue_PresentationDef,
		{
			ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::FireRate, 6.5f, 0.01f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 4.2f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::Range, 2200.f, 0.f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, 8.f, 0.1f },
			ly::GameplayAttribute{ ly::CommonAttributeIds::AreaRadius, 0.f, 0.f },
			ly::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f }
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		true,
		{
			PrimaryWeaponLevelStep{
				{
					ly::AttributeModifier{ ly::CommonAttributeIds::Damage, 2.f },
					ly::AttributeModifier{ ly::CommonAttributeIds::Range, 120.f }
				}
			},
			PrimaryWeaponLevelStep{
				{
					ly::AttributeModifier{ ly::CommonAttributeIds::FireRate, 0.5f },
					ly::AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 40.f }
				}
			},
			PrimaryWeaponLevelStep{
				{
					ly::AttributeModifier{ ly::CommonAttributeIds::Damage, 3.f },
					ly::AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 1.f }
				}
			}
		}
	);

	static const PrimaryWeaponDefinition Enemy_Vanguard_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Enemy_Vanguard_Elite_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Enemy_TwinBlade_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Enemy_Hexagon_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Enemy_UFO_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Boss_Base_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Boss_ThreeWay_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Boss_FrontalSweep_PrimaryWeaponDef(
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

	static const PrimaryWeaponDefinition Boss_LastStage_PrimaryWeaponDef(
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
