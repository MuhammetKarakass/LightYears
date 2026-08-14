#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/AreaTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Relay Prism owns its projectile presentation separately from the capture
	// telegraph because the projectile is visible during the delivery phase,
	// while the area is created only after it reaches the destination.
	struct RelayPrismProjectileVisualDefinition
	{
		sf::Color coreColor{ 255, 185, 255, 250 };
		sf::Color glowColor{ 235, 70, 255, 145 };
		sf::Color trailColor{ 180, 80, 255, 190 };
		float coreRadius = 7.f;
		float glowRadius = 19.f;
		float trailLength = 34.f;
		float trailWidth = 9.f;
		float pulseSpeed = 18.f;
	};

	struct RelayPrismPresentationProfile
	{
		sas::ContentId profileId;
		RelayPrismProjectileVisualDefinition projectile;
		AreaTelegraphVisualDefinition relayTelegraph;
	};

	bool RegisterRelayPrismPresentationProfiles();
}
