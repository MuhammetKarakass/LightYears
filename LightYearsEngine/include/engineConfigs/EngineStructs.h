#pragma once

#include <SFML/Graphics.hpp>
#include "framework/Core.h"
#include "framework/MathUtility.h"
#include <string>

struct PointLightDefinition
{
	std::string shaderPath;
	sf::Color color;
	float intensity;
	sf::Vector2f size;
	bool shouldStretch;
	bool complexTrail;
	float currentStretchFactor;
	float currentRotationOffset;
	float taperAmount;
	float edgeSoftness;
	float shapeRoundness;

	PointLightDefinition(
		const std::string& inShaderPath = "SpaceShooterRedux/Shaders/point_light.frag",
		const sf::Color& inColor = sf::Color::White,
		float inIntensity = 1.f,
		const sf::Vector2f& inSize = { 10, 10.f },
		bool inShouldStretch = false,
		bool inComplexTrail = false,
		float inTaperAmount = 0.f,
		float inEdgeSoftness = 1.f,
		float inShapeRoundness = 1.f
	)
		: shaderPath(inShaderPath)
		, color(inColor)
		, intensity(inIntensity)
		, size(inSize)
		, shouldStretch(inShouldStretch)
		, complexTrail(inComplexTrail)
		, currentStretchFactor(1.f)
		, currentRotationOffset(0.f)
		, taperAmount(inTaperAmount)
		, edgeSoftness(inEdgeSoftness)
		, shapeRoundness(inShapeRoundness
		)
	{
	}
};

struct BackgroundLayerDefinition
{
	std::string texturePath;
	bool hasLight;
	PointLightDefinition lightDef; // Iþýk ayarlarýný buradan alacaðýz
	float lightScaleRatio;         // Gezegen boyutuna göre ýþýk oraný

	BackgroundLayerDefinition(const std::string& path, const PointLightDefinition& def, float scaleRatio = 1.3f, bool hasLight = true)
		: texturePath(path), hasLight(hasLight), lightDef(def), lightScaleRatio(scaleRatio)
	{
	}

	BackgroundLayerDefinition(const std::string& path)
		: texturePath(path), hasLight(false), lightDef(), lightScaleRatio(1.f)
	{
	}
};