#pragma once

#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct AstralSurgeProjectileVisualDefinition
	{
		sf::Color outerColor{ 130, 70, 255, 185 };
		sf::Color coreColor{ 245, 235, 255, 255 };
		sf::Color glowColor{ 160, 105, 255, 130 };
		sf::Color impactColor{ 240, 220, 255, 230 };
		float crescentRadius = 140.f;
		float crescentThickness = 34.f;
		float crescentArcDegrees = 160.f;
		float glowThickness = 58.f;
		float pulseSpeed = 12.f;
		float impactRadius = 34.f;
		float impactDuration = 0.10f;
	};

	struct AstralSurgePresentationProfile
	{
		sas::ContentId profileId;
		AstralSurgeProjectileVisualDefinition projectile;
	};

	bool RegisterAstralSurgePresentationProfiles();
}
