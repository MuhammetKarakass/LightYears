#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct NullPulsePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color pulseColor{ 100, 225, 255, 235 };
		sf::Color projectileBreakColor{ 175, 245, 255, 245 };
		sf::Color stunColor{ 110, 170, 255, 230 };
		float pulseDuration = 0.24f;
		float projectileBreakDuration = 0.14f;
		float stunFlashDuration = 0.20f;
	};

	bool RegisterNullPulsePresentationProfiles();
}
