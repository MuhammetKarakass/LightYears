#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct NanoPlaguePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color infectionColor{ 95, 255, 140, 125 };
		sf::Color injectionColor{ 175, 255, 205, 230 };
		sf::Color spreadColor{ 95, 255, 140, 200 };
		float auraRadius = 25.f;
		float injectionDuration = 0.18f;
		float spreadDuration = 0.34f;
	};

	bool RegisterNanoPlaguePresentationProfiles();
}
