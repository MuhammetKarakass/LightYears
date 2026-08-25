#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct CrescentReaverVisualDefinition
	{
		sf::Color bladeColor{ 220, 235, 255, 245 };
		sf::Color edgeColor{ 85, 190, 255, 225 };
		sf::Color glowColor{ 45, 155, 255, 115 };
		sf::Color impactColor{ 235, 250, 255, 245 };
		sf::Color impactRingColor{ 80, 185, 255, 230 };
		float radius = 24.f;
		float bladeThickness = 8.f;
		float glowRadius = 30.f;
		float spinDegreesPerSecond = 720.f;
		float trailLength = 34.f;
		float trailWidth = 8.f;
		float impactRadius = 22.f;
		float impactDuration = 0.14f;
		float impactRingThickness = 3.f;
		float dissipationDuration = 0.24f;
	};

	struct CrescentReaverPresentationProfile
	{
		sas::ContentId profileId;
		CrescentReaverVisualDefinition visual;
	};

	bool RegisterCrescentReaverPresentationProfiles();
}
