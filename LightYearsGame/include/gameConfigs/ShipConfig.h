#pragma once

#include "player/Reward.h"
#include "gameConfigs/PointLightConfig.h"
#include "gameConfigs/ShipStructs.h"
#include "gameConfigs/WeaponConfig.h"
#include "VFX/Explosion.h"

namespace ShipData
{
	static const ShipDefinition Ship_Player_Fighter(
		"SpaceShooterRedux/PNG/playerShip1_blue.png",
		100.f,
		sf::Vector2f{ 350.f, 350.f },
		25.f,
		0.f,
		(int)ly::ExplosionType::Medium,
		{
			EngineMount{ {-22.f, -12.f}, LightingData::Engine_Cyan_PointLightDef },
			EngineMount{ {+22.f, -12.f}, LightingData::Engine_Cyan_PointLightDef }
		},
		{},
		WeaponData::Fighter_PrimaryWeaponDef,
		ShipMovementAttributes{
			650.f,
			190.f,
			270.f,
			400.f,
			5.f,
			0.36f,
			520.f,
			12.f,
			32.f
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
			EngineMount{ {0.f,-10.f},LightingData::Engine_Yellow_PointLightDef },
		},
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardLife, 0.05f},
			{ ly::CreateRewardShield, 0.08f }
		},
		WeaponData::Enemy_Vanguard_PrimaryWeaponDef
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
			EngineMount{ {0.f,20.f},LightingData::Engine_Orange_PointLightDef },
		},
		{
			{ ly::CreateRewardHealth, 0.2f },
			{ ly::CreateRewardLife, 0.01f},
			{ ly::CreateRewardShield, 0.08f }
		},
		WeaponData::Enemy_Vanguard_Elite_PrimaryWeaponDef
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
		WeaponData::Enemy_TwinBlade_PrimaryWeaponDef
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
		WeaponData::Enemy_Hexagon_PrimaryWeaponDef
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
		WeaponData::Enemy_UFO_PrimaryWeaponDef
	);
}
