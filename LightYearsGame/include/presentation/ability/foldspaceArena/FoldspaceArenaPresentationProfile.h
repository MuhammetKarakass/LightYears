#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// This profile deliberately contains only the fold-space look. Gameplay
	// geometry, damage and duration stay in the ability's runtime attributes.
	struct FoldspaceArenaPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color projectileCoreColor{ 190, 125, 255, 245 };
		sf::Color projectileGlowColor{ 105, 220, 255, 145 };
		sf::Color boundaryColor{ 145, 215, 255, 220 };
		sf::Color boundaryGlowColor{ 118, 105, 255, 110 };
		sf::Color ghostColor{ 175, 235, 255, 112 };
		float projectileRadius = 10.f;
		float boundaryThickness = 2.5f;
		float boundaryPulseSpeed = 3.5f;
		float boundaryPulseAmount = 0.18f;
	};

	bool RegisterFoldspaceArenaPresentationProfiles();
}
