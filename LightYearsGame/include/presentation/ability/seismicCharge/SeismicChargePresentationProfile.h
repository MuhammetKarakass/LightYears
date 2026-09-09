#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// This profile owns only Seismic Charge's visual identity. Gameplay radius,
	// timing and damage remain actor attributes in shipped ability content.
	struct SeismicChargePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color bombOuterColor{ 80, 190, 255, 190 };
		sf::Color bombCoreColor{ 220, 250, 255, 255 };
		sf::Color telegraphColor{ 75, 180, 255, 125 };
		sf::Color shockwaveColor{ 145, 225, 255, 235 };
		float bombRadius = 18.f;
		float maximumRangeOutlineThickness = 2.f;
		float shockwaveOutlineThickness = 9.f;
	};

	bool RegisterSeismicChargePresentationProfiles();
}
