#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct CombatSentryTurretPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color bodyColor{ 60, 150, 235, 230 };
		sf::Color coreColor{ 170, 235, 255, 255 };
		sf::Color barrelColor{ 130, 210, 255, 255 };
		float bodyRadius = 26.f;
		float barrelLength = 36.f;
		float barrelWidth = 10.f;
	};

	struct CombatSentryProjectilePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color coreColor{ 185, 245, 255, 255 };
		sf::Color trailColor{ 55, 160, 255, 150 };
		float coreRadius = 6.f;
		float trailLength = 26.f;
	};

	bool RegisterCombatSentryPresentationProfiles();
}
