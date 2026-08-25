#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct FrostMaelstromPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color outerColor{ 130, 220, 255, 105 };
		sf::Color innerColor{ 205, 250, 255, 72 };
		sf::Color particleColor{ 175, 240, 255, 190 };
		int ringCount = 5;
		int particleCount = 18;
		float ringThickness = 3.f;
		float particleRadius = 3.f;
		float visualRotationSpeed = 3.5f;
		float pulseSpeed = 5.f;
	};

	bool RegisterFrostMaelstromPresentationProfiles();
}
