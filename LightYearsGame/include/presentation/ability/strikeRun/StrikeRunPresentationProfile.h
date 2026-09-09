#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct StrikeRunTelegraphVisualDefinition
	{
		sf::Color lineColor{ 80, 165, 255, 150 };
		sf::Color linePulseColor{ 155, 225, 255, 230 };
		sf::Color markerFillColor{ 35, 120, 255, 55 };
		sf::Color markerOutlineColor{ 115, 210, 255, 220 };
		float lineThickness = 6.f;
		float markerOutlineThickness = 3.f;
		float pulseSpeed = 7.f;
	};

	struct StrikeRunExplosionVisualDefinition
	{
		sf::Color outerFillColor{ 30, 120, 255, 95 };
		sf::Color outerOutlineColor{ 120, 215, 255, 240 };
		sf::Color shockwaveColor{ 225, 250, 255, 240 };
		sf::Color craftColor{ 190, 235, 255, 235 };
		float duration = 0.30f;
		float shockwaveThickness = 5.f;
		float craftLength = 70.f;
		float craftWidth = 20.f;
	};

	struct StrikeRunPresentationProfile
	{
		sas::ContentId profileId;
		StrikeRunTelegraphVisualDefinition telegraph;
		StrikeRunExplosionVisualDefinition explosion;
	};

	bool RegisterStrikeRunPresentationProfiles();
}
