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

#pragma region Ammo Definitions

	static const BulletDefinition Laser_Blue_BulletDef(
		"SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
		500.f,
		10.f,
		Laser_Blue_PointLightDef,
		{ 0.f,38.f }
		
	);

	static const BulletDefinition Laser_Red_BulletDef(
		"SpaceShooterRedux/PNG/Lasers/laserRed01.png",
		500.f,
		10.f,
		Laser_Red_PointLightDef,
		{ 0.f,38.f }
	);

	static const BulletDefinition Laser_Green_BulletDef(
		"SpaceShooterRedux/PNG/Lasers/laserGreen11.png",
		500.f,
		10.f,
		Laser_Green_PointLightDef,
		{ 0.f,38.f }
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
		Laser_Blue_BulletDef,
		true,
		sf::Vector2f{ 0.f, 50.f },      // weaponOffset
		0.25f,      // weaponCooldown
		{}        // rewards (player için boş)
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
		Laser_Red_BulletDef,
		true,
		sf::Vector2f{ 0.f,40.f },
		.9f,
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardThreeWayShooter, 0.12f },
			{ ly::CreateRewardFrontalWiper, 0.08f },
			{ ly::CreateRewardLife, 0.05f}
		}
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
		Laser_Red_BulletDef,
		true,
		sf::Vector2f{ 0.f,40.f },
		0.75f,
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardThreeWayShooter, 0.1f },
			{ ly::CreateRewardFrontalWiper, 0.06f },
			{ ly::CreateRewardLife, 0.01f}
		}
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
		Laser_Red_BulletDef,
		true,
		sf::Vector2f{ 0.f,40.f },
		1.f,
		{
			{ ly::CreateRewardHealth, 0.25f },
			{ ly::CreateRewardThreeWayShooter, 0.15f },
			{ ly::CreateRewardFrontalWiper, 0.10f },
			{ ly::CreateRewardLife, 0.05f}
		}
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
		Laser_Red_BulletDef,
		true,
		sf::Vector2f{ 0.f,0.f },
		.75f,
		{
			{ ly::CreateRewardHealth, 0.3f },
			{ ly::CreateRewardThreeWayShooter, 0.2f },
			{ ly::CreateRewardFrontalWiper, 0.15f },
			{ ly::CreateRewardLife, 0.05f}
		}
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
		Laser_Red_BulletDef,
		true,
		sf::Vector2f{ 0.f, 100.f },
		.9f,
		{
			{ ly::CreateRewardHealth, 0.3f },
			{ ly::CreateRewardThreeWayShooter, 0.25f },
			{ ly::CreateRewardFrontalWiper, 0.15f },
			{ ly::CreateRewardLife, 0.08f }
		}
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