#pragma once

#include "player/Reward.h"
#include "gameConfigs/GameplayStructs.h"
#include "engineConfigs/EngineStructs.h"
#include "VFX/Explosion.h"

namespace GameData
{


#pragma region Point Light Definitions


	static const PointLightDefinition Laser_Blue_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 75,244,255,200 },
		0.3f,
		sf::Vector2f{ 18.f,80.f },
		false,
		false,
		0.0f,
		.4f,
		.7f
	);

	static const PointLightDefinition Laser_Red_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255,110,110,200 },
		0.3f,
		sf::Vector2f{ 18.f,80.f },
		false,
		false,
		0.0f,
		.4f,
		.7f
	);

	static const PointLightDefinition Laser_Green_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 110,255,110,200 },
		0.3f,
		sf::Vector2f{ 18.f,80.f },
		false,
		false,
		0.0f,
		.4f,
		.7f
	);

	static const PointLightDefinition Engine_Cyan_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 0,183,235,200 }, 1.5f, sf::Vector2f{ 50.f,100.f },
		true,
		true,
		.75f,
		1.f,
		0.2f
	);

	static const PointLightDefinition Engine_Yellow_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255, 255, 0, 200 }, 1.5f, sf::Vector2f{ 50.f,100.f },
		true,
		true,
		.75f,
		1.f,
		0.2f
	);

	static const PointLightDefinition Engine_Orange_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255, 165, 0, 200 }, 1.5f, sf::Vector2f{ 50.f,100.f },
		true,
		true,
		.75f,
		1.f,
		0.2f
	);

	static const PointLightDefinition Engine_Red_PointLightDef(
		"SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255, 0, 0, 200 },
		1.5f,
		sf::Vector2f{ 50.f,100.f },
		true,
		true,
		.75f,
		1.f,
		0.2f
	);

#pragma endregion

#pragma region Weapon Definitions

	static const WeaponPresentationDefinition Laser_Blue_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
		Laser_Blue_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const WeaponPresentationDefinition Laser_Red_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserRed01.png",
		Laser_Red_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const WeaponPresentationDefinition Laser_Green_PresentationDef(
		"SpaceShooterRedux/PNG/Lasers/laserGreen11.png",
		Laser_Green_PointLightDef,
		{ 0.f,38.f },
		1.f
	);

	static const PrimaryWeaponDefinition Fighter_PrimaryWeaponDef(
		"FighterPulseCannon",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Blue_PresentationDef,
		PrimaryWeaponAttributes{
			10.f,    // damage: current default blue laser damage.
			6.5f,    // shotsPerSecond: continuous primary fire, not player-facing cooldown.
			500.f,   // projectileSpeed: current default blue laser speed.
			4.2f,    // projectileLifeTime: matches current arena-safe bullet lifetime.
			2200.f,  // projectileMaxTravelDistance: matches current arena-safe bullet range.
			8.f,     // projectileCollisionRadius: placeholder until projectile collision shape is data-driven.
			0.f,     // projectileAreaRadius: zero means no area damage.
			0.f,     // spreadAngle: single focused shot.
			1.f,     // projectilesPerShot: single projectile stream.
			0.f,     // pierceCount: no piercing by default.
			0.f,     // beamRange: unused for projectile delivery.
			0.f      // beamWidth: unused for projectile delivery.
		},
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 50.f }, 0.f } },
		PrimaryWeaponFirePattern::Single,
		true
	);

	static const PrimaryWeaponDefinition Enemy_Vanguard_PrimaryWeaponDef(
		"EnemyVanguardBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 1.1f, 500.f, 3.6f, 1900.f, 8.f },
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 40.f }, 0.f } }
	);

	static const PrimaryWeaponDefinition Enemy_Vanguard_Elite_PrimaryWeaponDef(
		"EnemyVanguardEliteBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 1.35f, 500.f, 3.6f, 1900.f, 8.f },
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 40.f }, 0.f } }
	);

	static const PrimaryWeaponDefinition Enemy_TwinBlade_PrimaryWeaponDef(
		"EnemyTwinBladeDualBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 1.f, 400.f, 3.6f, 1900.f, 8.f },
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -20.f, 40.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 20.f, 40.f }, 0.f }
		}
	);

	static const PrimaryWeaponDefinition Enemy_Hexagon_PrimaryWeaponDef(
		"EnemyHexagonRadialBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 1.35f, 400.f, 3.6f, 1900.f, 8.f },
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
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 1.1f, 500.f, 3.6f, 1900.f, 8.f },
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ 35.f, 20.f }, 60.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ -35.f, 20.f }, -60.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, -40.f }, 180.f }
		}
	);

	static const PrimaryWeaponDefinition Boss_Base_PrimaryWeaponDef(
		"BossBaseDualBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 2.f, 500.f, 3.6f, 1900.f, 8.f },
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -50.f, 50.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 50.f, 50.f }, 0.f }
		}
	);

	static const PrimaryWeaponDefinition Boss_ThreeWay_PrimaryWeaponDef(
		"BossThreeWayBlaster",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 0.5f, 500.f, 3.6f, 1900.f, 8.f, 0.f, 60.f, 3.f },
		{ WeaponMuzzleDefinition{ sf::Vector2f{ 0.f, 100.f }, 0.f } },
		PrimaryWeaponFirePattern::Spread
	);

	static const PrimaryWeaponDefinition Boss_FrontalSweep_PrimaryWeaponDef(
		"BossFrontalSweep",
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 0.33f, 500.f, 3.6f, 1900.f, 8.f },
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
		PrimaryWeaponDeliveryType::Projectile,
		Laser_Red_PresentationDef,
		PrimaryWeaponAttributes{ 10.f, 2.f, 500.f, 3.6f, 1900.f, 8.f },
		{
			WeaponMuzzleDefinition{ sf::Vector2f{ -150.f, 50.f }, 0.f },
			WeaponMuzzleDefinition{ sf::Vector2f{ 150.f, 50.f }, 0.f }
		}
	);

#pragma endregion

#pragma region Ship Definitions

	static const ShipDefinition Ship_Player_Fighter(
		"SpaceShooterRedux/PNG/playerShip1_blue.png",
		100.f,   // health
		sf::Vector2f{ 350.f, 350.f },   // speed
		25.f,           // collisionDamage
		0.f,// scoreAmt
		(int)ly::ExplosionType::Medium,
		{
			EngineMount{ {-22.f, -12.f}, Engine_Cyan_PointLightDef },
			EngineMount{ {+22.f, -12.f}, Engine_Cyan_PointLightDef }
		},
		{},
		Fighter_PrimaryWeaponDef,
		ShipMovementAttributes{
			650.f,   // forwardThrust: reduced so arena speed does not run away too quickly
			190.f,   // reverseThrust: intentionally weaker than forward
			270.f,   // strafeThrust: side thrust remains lower than forward
			400.f,   // angularTurnSpeed: maximum degrees per second
			5.f,    // angularTurnResponsiveness: lower is smoother, higher is snappier
			0.36f,   // linearDamping: shorter drift than 0.45, but still keeps some glide
			520.f,    // maxSpeed: terminal velocity for thrust/drift movement
			12.f,     // inputResponsiveness: smooths thrust input without making controls feel delayed
			32.f      // mouseAimDeadZone: prevents close-cursor rotation jitter around the ship
		}
	);

	static const ShipDefinition Ship_Enemy_Vanguard(
		"SpaceShooterRedux/PNG/Enemies/enemyBlack1.png",
		60.f,
		sf::Vector2f{ 0.f,200.f },
		50.f,
		10.f,
		(int)ly::ExplosionType::Medium,
		{
			EngineMount{ {0.f,-10.f},Engine_Yellow_PointLightDef },
		},
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardLife, 0.05f},
			{ ly::CreateRewardShield, 0.08f }
		},
		Enemy_Vanguard_PrimaryWeaponDef
	);

	static const ShipDefinition Ship_Enemy_Vanguard_Elite
	(
		"SpaceShooterRedux/PNG/Enemies/enemyBlue1.png",
		200.f,
		sf::Vector2f{ 0.f,175.f },
		75.f,
		25.f,
		(int)ly::ExplosionType::Heavy,
		{
			EngineMount{ {0.f,20.f},Engine_Orange_PointLightDef },
		},
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardLife, 0.01f},
			{ ly::CreateRewardShield, 0.08f }
		},
		Enemy_Vanguard_Elite_PrimaryWeaponDef
	);

	static const ShipDefinition Ship_Enemy_TwinBlade
	(
		"SpaceShooterRedux/PNG/Enemies/enemyBlack3.png",
		60.f,
		sf::Vector2f{ 0.f,175.f },
		50.f,
		20.f,
		(int)ly::ExplosionType::Medium,
		{
			EngineMount{ {0.f,-30.f},PointLightDefinition("SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255, 255, 0, 200 },
		1.5f,
		sf::Vector2f{ 45.f,60.f },
		true,
		true,
		1.f,
		1.f,
		0.2f)
			}
		},
		{
			{ ly::CreateRewardHealth, 0.25f },
			{ ly::CreateRewardLife, 0.05f},
			{ ly::CreateRewardShield, 0.08f }
		},
		Enemy_TwinBlade_PrimaryWeaponDef
	);

	static const ShipDefinition Ship_Enemy_Hexagon
	(
		"SpaceShooterRedux/PNG/Enemies/enemyBlack4.png",
		100.f,
		sf::Vector2f{ 0.f,125.f },
		60.f,
		30.f,
		(int)ly::ExplosionType::Heavy,
		{
			EngineMount{ {0.f,-25.f},PointLightDefinition("SpaceShooterRedux/Shaders/point_light.frag",
		sf::Color{ 255, 255, 0, 200 },
		1.5f,
		sf::Vector2f{ 20.f,75.f },
		true,
		true,
		1.f,
		1.f,
		0.2f)
			}
		},
		{
			{ ly::CreateRewardHealth, 0.3f },
			{ ly::CreateRewardLife, 0.05f},
			{ ly::CreateRewardShield, 0.10f }
		},
		Enemy_Hexagon_PrimaryWeaponDef
	);

	static const ShipDefinition Ship_Enemy_UFO
	(
		"SpaceShooterRedux/PNG/Enemies/ufoBlack.png",
		80.f,
		sf::Vector2f{ 0.f,300.f },
		80.f,
		40.f,
		(int)ly::ExplosionType::Heavy,
		{
			EngineMount{
				{0.f, 70.f},
				PointLightDefinition(
					"SpaceShooterRedux/Shaders/point_light.frag",
					sf::Color{ 255, 255, 0, 200},
					1.5f,
					sf::Vector2f{ 140.f, 140.f },
					false,
					false,
					0.0f,
					0.7f,
					0.0f
				)
			}
		},
		{
			{ ly::CreateRewardHealth, 0.3f },
			{ ly::CreateRewardLife, 0.08f },
			{ ly::CreateRewardShield, 0.10f }
		},
		Enemy_UFO_PrimaryWeaponDef
	);
#pragma endregion

#pragma region Planets

	namespace Environment
	{
		static const BackgroundLayerDefinition Star_Orange
		(
			"SpaceShooterRedux/PNG/Planets/Planet3.png",
			PointLightDefinition
			(
				"SpaceShooterRedux/Shaders/point_light.frag",
				sf::Color{ 255, 165, 0, 220 },
				1.f,
				sf::Vector2f{ 1.f, 1.f },
				false,
				false,
				0.0f,
				1.f,
				0.0f
			),
			1.25f
		);

		static const BackgroundLayerDefinition Planet_Earth_Blue
		(
			"SpaceShooterRedux/PNG/Planets/Planet1.png",
			PointLightDefinition
			(
				"SpaceShooterRedux/Shaders/point_light.frag",
				sf::Color{ 170, 170, 255, 200 },
				1.f,
				sf::Vector2f{ 1.f, 1.f },
				false,
				false,
				0.0f,
				1.f,
				0.0f
			),
			1.2f
		);

		static const BackgroundLayerDefinition Planet_Green
		(
			"SpaceShooterRedux/PNG/Planets/Planet7.png",
			PointLightDefinition
			(
				"SpaceShooterRedux/Shaders/point_light.frag",
				sf::Color{ 170, 255, 170, 200 },
				1.f,
				sf::Vector2f{ 1.f, 1.f },
				false,
				false,
				0.0f,
				1.f,
				0.0f
			)
			, 1.2f
		);

		static const BackgroundLayerDefinition Planet_Orange
		(
			"SpaceShooterRedux/PNG/Planets/Planet6.png",
			PointLightDefinition
			(
				"SpaceShooterRedux/Shaders/point_light.frag",
				sf::Color{ 255, 140, 140, 200 },
				1.f,
				sf::Vector2f{ 1.f, 1.f },
				false,
				false,
				0.0f,
				1.f,
				0.0f
			)
			, 1.3f
		);

		static const BackgroundLayerDefinition Planet_Blue
		(
			"SpaceShooterRedux/PNG/Planets/Planet2.png"
		);

		static const BackgroundLayerDefinition Meteor1
		(
			"SpaceShooterRedux/PNG/Planets/Planet4.png"
		);

		static const BackgroundLayerDefinition Meteor2
		(
			"SpaceShooterRedux/PNG/Planets/Planet5.png"
		);
	}

#pragma endregion
}
