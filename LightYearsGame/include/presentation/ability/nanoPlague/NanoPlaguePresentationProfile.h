#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct NanoPlaguePresentationProfile
	{
		sas::ContentId profileId;
		// Nano Plague uses the same electric visual language as its damage type.
		sf::Color infectionColor{ 70, 185, 255, 145 };
		sf::Color injectionColor{ 220, 250, 255, 240 };
		sf::Color spreadColor{ 105, 210, 255, 220 };
		float auraRadius = 25.f;
		float injectionDuration = 0.18f;
		float spreadDuration = 0.34f;
	};

	bool RegisterNanoPlaguePresentationProfiles();
}
