#pragma once

#include "engineConfigs/EngineStructs.h"

namespace EnvironmentData
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
