#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Chain Lightning owns the electric arc's presentation values in its
	// feature-local typed profile. Gameplay code only supplies the resolved
	// target chain; it does not invent global visual configuration.
	struct ChainLightningPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color outerColor{ 50, 170, 255, 210 };
		sf::Color coreColor{ 215, 250, 255, 255 };
		sf::Color impactColor{ 235, 255, 255, 245 };
		float impactDuration = 0.14f;
		float lingerDuration = 0.12f;
		float jitter = 18.f;
		float pulseSpeed = 22.f;
		int pathPointCount = 8;
	};

	bool RegisterChainLightningPresentationProfiles();
}
