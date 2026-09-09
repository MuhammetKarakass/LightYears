#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Typed, family-local presentation data. Gameplay range, hit width and tick
	// cadence remain actor attributes and are never visual profile switches.
	struct ArcScythesPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color outerColor{ 35, 135, 255, 125 };
		sf::Color coreColor{ 205, 248, 255, 245 };
		float outerHalfThickness = 14.f;
		float coreHalfThickness = 3.f;
		float emitterOffset = 34.f;
		float pulseSpeed = 13.f;
		float arcAmplitude = 7.f;
	};

	bool RegisterArcScythesPresentationProfiles();
}
