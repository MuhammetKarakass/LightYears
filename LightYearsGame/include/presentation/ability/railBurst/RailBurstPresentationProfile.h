#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct RailBurstVisualDefinition
	{
		sf::Color outerColor{ 70, 190, 255, 185 };
		sf::Color coreColor{ 245, 255, 255, 255 };
		sf::Color glowColor{ 65, 205, 255, 120 };
		sf::Color impactColor{ 225, 250, 255, 245 };
		sf::Color impactRingColor{ 80, 205, 255, 230 };
		float bodyLength = 190.f;
		float bodyWidth = 7.f;
		float coreWidth = 1.8f;
		float trailLength = 360.f;
		float trailWidth = 24.f;
		float glowRadius = 7.f;
		float pulseSpeed = 70.f;
		float headLength = 28.f;
		float railSeparation = 9.f;
		float railThickness = 2.f;
		float afterimageSpacing = 46.f;
		float afterimageDecay = 0.42f;
		int afterimageCount = 3;
		float impactRadius = 18.f;
		float impactDuration = 0.12f;
		float impactRingThickness = 3.f;
	};

	struct RailBurstPresentationProfile
	{
		sas::ContentId profileId;
		RailBurstVisualDefinition visual;
	};

	bool RegisterRailBurstPresentationProfiles();
}
