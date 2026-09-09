#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct AegisReaverVisualDefinition
	{
		sf::Color coreColor{ 110, 225, 255, 250 };
		sf::Color edgeColor{ 225, 250, 255, 245 };
		sf::Color glowColor{ 40, 165, 255, 105 };
		float radius = 18.f;
		float glowRadius = 30.f;
		float spinDegreesPerSecond = 900.f;
		float trailLength = 34.f;
		float trailWidth = 8.f;
	};

	struct AegisReaverPresentationProfile
	{
		sas::ContentId profileId;
		AegisReaverVisualDefinition visual;
	};

	bool RegisterAegisReaverPresentationProfiles();
}
