#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct EmberDroneVisualDefinition
	{
		sf::Color glowColor{ 255, 120, 30, 140 };
		sf::Color bodyColor{ 255, 180, 50, 240 };
		sf::Color coreColor{ 255, 240, 200, 255 };

		float bodyRadius = 6.f;
		float coreRadius = 2.2f;
		float glowRadius = 15.f;
		float expiryFadeDuration = 0.6f;

		// Milestone 2: Thin orange/red pulse and impact presentation
		sf::Color pulseColor{ 255, 80, 20, 200 };
		sf::Color impactColor{ 240, 40, 10, 220 };
		float pulseRadius = 18.f;
		float pulseDuration = 0.12f;
		float impactRadius = 10.f;
		float impactDuration = 0.12f;
		float pulseOutlineThickness = 1.2f;
	};

	struct EmberSwarmPresentationProfile
	{
		sas::ContentId profileId;
		EmberDroneVisualDefinition visual;
	};

	bool RegisterEmberSwarmPresentationProfiles();
}
