#pragma once

#include "gameplay/attributes/AttributeIds.h"

#include "player/Reward.h"
#include "gameConfigs/presentation/PointLightConfig.h"
#include "gameConfigs/ship/ShipStructs.h"
#include "VFX/Explosion.h"

namespace ShipData
{
	static const ShipDefinition Ship_Player_Fighter(
		"SpaceShooterRedux/PNG/playerShip1_blue.png",
		250.f,
		sf::Vector2f{ 350.f, 350.f },
		25.f,
		0.f,
		(int)vega::ExplosionType::Medium,
		{
			EngineMount{ {-22.f, -12.f}, LightingData::Engine_Cyan_PointLightDef },
			EngineMount{ {+22.f, -12.f}, LightingData::Engine_Cyan_PointLightDef }
		},
		{},
		"Weapon.Projectile.FighterRapidLaser.Basic",
		ShipMovementAttributes{
			650.f,
			190.f,
			270.f,
			400.f,
			5.f,
			0.70f,
			520.f,
			12.f,
			32.f
		},
		ShipEnergyAttributes{
			35.f,
			140.f,
			20.f,
			6.f,
			6.f,
			100.f,
			3.f,
			6.f,
			1.5f,
			1.55f,
			1.80f,
			33.f,
			0.22f,
			0.45f,
			0.90f
		},
		ShipProgressionDefinition{
			100.f,
			1.25f,
			{
				{ vega::OwnerAttributeIds::MaxHealth, 40.f },
				{ vega::OwnerAttributeIds::AttackPower, 3.f },
				{ vega::OwnerAttributeIds::EnergyPower, 2.f },
				{ vega::OwnerAttributeIds::Armor, 2.f },
				{ vega::OwnerAttributeIds::Luck, 1.f }
			}
		},
		0.f,
		vega::ControlTargetClass::Normal,
		{
			{ vega::OwnerAttributeIds::AttackPower, 39.f },
			{ vega::OwnerAttributeIds::Armor, 2.f }
		}
	);

	static const ShipDefinition Ship_Enemy_Hexagon
	(
		"SpaceShooterRedux/PNG/Enemies/enemyBlack4.png",
		100.f,
		sf::Vector2f{ 0.f,125.f },
		60.f,
		30.f,
		(int)vega::ExplosionType::Heavy,
		{
			EngineMount{ {0.f,-25.f},vega::PointLightDefinition("SpaceShooterRedux/Shaders/point_light.frag",
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
			{ vega::CreateRewardHealth, 0.3f },
			{ vega::CreateRewardLife, 0.05f},
			{ vega::CreateRewardShield, 0.10f }
		},
		""
	);
}
