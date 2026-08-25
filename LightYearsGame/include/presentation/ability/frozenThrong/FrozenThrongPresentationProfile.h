#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Frozen Throng owns one typed profile for the Husk projectile. The actor
	// consumes this small presentation contract and remains independent from a
	// global visual configuration struct.
	struct FrozenThrongPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color coreColor{ 190, 245, 255, 255 };
		sf::Color glowColor{ 95, 190, 255, 185 };
		float coreRadius = 8.f;
		float glowRadius = 16.f;
		float pulseSpeed = 8.f;
	};

	bool RegisterFrozenThrongPresentationProfiles();
}
