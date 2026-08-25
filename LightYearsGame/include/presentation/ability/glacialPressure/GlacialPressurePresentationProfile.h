#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

#include <array>

namespace ly
{
	struct GlacialPressurePresentationProfile
	{
		sas::ContentId profileId;
		float range = 700.f;
		float halfAngleDegrees = 22.f;
		int segmentCount = 5;
		std::array<sf::Color, 5> segmentColors{
			sf::Color{ 190, 250, 255, 130 },
			sf::Color{ 150, 230, 255, 112 },
			sf::Color{ 115, 205, 255, 94 },
			sf::Color{ 90, 175, 245, 76 },
			sf::Color{ 75, 145, 220, 58 }
		};
		sf::Color separatorColor{ 220, 255, 255, 150 };
		sf::Color completionColor{ 245, 255, 255, 220 };
		float separatorThickness = 1.5f;
		float pulseSpeed = 8.f;
		float completionFeedbackDuration = 0.25f;
	};

	bool RegisterGlacialPressurePresentationProfiles();
}
