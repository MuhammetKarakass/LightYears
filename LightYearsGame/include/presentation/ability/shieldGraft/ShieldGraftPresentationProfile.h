#pragma once

#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct ShieldGraftPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color outerRingColor{ 80, 200, 255, 220 };
		sf::Color innerFlashColor{ 95, 245, 160, 230 };
		float visualDuration = 0.35f;
		float startRadius = 80.f;
		float endRadius = 10.f;
		float ringThickness = 3.f;
	};

	bool RegisterShieldGraftPresentationProfiles();
}
