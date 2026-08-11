#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct PhaseDriftPresentationProfile
	{
		sas::ContentId profileId;
		// Low alpha keeps the ship readable while still communicating the phase.
		sf::Color auraColor{ 95, 205, 255, 72 };
		sf::Color outlineColor{ 165, 235, 255, 105 };
		float radius = 52.f;
		float outlineThickness = 2.5f;
		float pulseSpeed = 6.f;
	};

	bool RegisterPhaseDriftPresentationProfiles();
}
