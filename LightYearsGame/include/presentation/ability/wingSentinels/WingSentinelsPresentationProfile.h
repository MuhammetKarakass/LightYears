#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct WingSentinelsPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color droneGlowColor{ 95, 180, 255, 100 };
		sf::Color droneBodyColor{ 105, 205, 255, 255 };
		sf::Color droneCoreColor{ 235, 250, 255, 255 };
		sf::Color projectileColor{ 145, 220, 255, 255 };
		sf::Color projectileGlowColor{ 80, 175, 255, 130 };
		float droneRadius = 11.f;
		float projectileRadius = 5.f;
	};

	bool RegisterWingSentinelsPresentationProfiles();
}
