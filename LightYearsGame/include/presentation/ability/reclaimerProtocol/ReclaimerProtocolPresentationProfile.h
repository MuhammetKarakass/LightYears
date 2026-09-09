#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct ReclaimerProtocolPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color capsuleColor{ 80, 220, 140, 240 };
		sf::Color innerColor{ 180, 255, 210, 255 };
		sf::Color pulseColor{ 120, 255, 170, 190 };
		sf::Color collectedFlashColor{ 255, 255, 255, 255 };
		float capsuleWidth = 14.f;
		float capsuleHeight = 26.f;
		float pulseSpeed = 6.f;
		float fastPulseSpeed = 16.f;
		float flashDuration = 0.12f;
	};

	bool RegisterReclaimerProtocolPresentationProfiles();
}