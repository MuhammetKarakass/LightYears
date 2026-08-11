#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

#include <string>

namespace ly
{
	struct GravityAnomalyProjectileVisualDefinition
	{
		sf::Color coreColor{ 185, 150, 255, 245 };
		sf::Color glowColor{ 105, 85, 255, 125 };
		sf::Color trailColor{ 85, 145, 255, 170 };
		float coreRadius = 9.f;
		float glowRadius = 22.f;
		float trailLength = 42.f;
		float trailWidth = 10.f;
		float pulseSpeed = 16.f;
	};

	struct GravityAnomalyFieldVisualDefinition
	{
		sf::Color centerColor{ 48, 16, 105, 210 };
		sf::Color innerRingColor{ 125, 85, 255, 210 };
		sf::Color outerRingColor{ 80, 180, 255, 180 };
		sf::Color boundaryColor{ 140, 115, 255, 145 };
		float boundaryThickness = 3.f;
		float innerRadiusRatio = 0.34f;
		float ringThickness = 3.f;
		float rotationSpeed = 110.f;
		float expansionDuration = 0.18f;
		float collapseDuration = 0.24f;
		int particleCount = 16;
	};

	struct GravityAnomalyProjectilePresentationProfile
	{
		sas::ContentId profileId;
		std::string texturePath;
		GravityAnomalyProjectileVisualDefinition visual;
	};

	struct GravityAnomalyFieldPresentationProfile
	{
		sas::ContentId profileId;
		GravityAnomalyFieldVisualDefinition visual;
	};

	bool RegisterGravityAnomalyPresentationProfiles();
}
