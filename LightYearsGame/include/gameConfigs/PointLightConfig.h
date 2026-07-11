#pragma once

#include "engineConfigs/EngineStructs.h"

namespace LightingData
{
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
}
