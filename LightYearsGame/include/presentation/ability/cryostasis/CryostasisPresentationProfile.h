#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Cryostasis owns its presentation values. The gameplay definition contains
	// only balance and mechanics, never render-specific knobs.
	struct CryostasisPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color shellColor{ 105, 210, 255, 115 };
		sf::Color crackColor{ 225, 250, 255, 220 };
		sf::Color fieldColor{ 110, 205, 255, 42 };
		float shellRadius = 48.f;
		float fieldRadius = 300.f;
		float shellPulseSpeed = 3.f;
		float breakDuration = 0.45f;
	};

	bool RegisterCryostasisPresentationProfiles();
}
