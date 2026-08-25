#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct ReturnProtocolPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color ringColor{ 105, 225, 255, 230 };
		sf::Color innerRingColor{ 190, 115, 255, 165 };
		float radius = 68.f;
		float ringThickness = 3.f;
		float pulseSpeed = 15.f;
	};

	bool RegisterReturnProtocolPresentationProfiles();
}
