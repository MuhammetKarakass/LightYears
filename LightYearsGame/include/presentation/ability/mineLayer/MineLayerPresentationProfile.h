#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct MineLayerVisualDefinition
	{
		sf::Color bodyColor{ 55, 145, 255, 235 };
		sf::Color coreColor{ 225, 250, 255, 255 };
		sf::Color triggerRingColor{ 70, 170, 255, 105 };
		sf::Color explosionColor{ 120, 210, 255, 220 };
		sf::Color explosionRingColor{ 225, 250, 255, 245 };
		float bodyRadius = 15.f;
		float coreRadius = 6.f;
		float triggerRingThickness = 2.f;
		float pulseSpeed = 4.f;
		float explosionDuration = 0.24f;
		float explosionEndScale = 1.45f;
		float explosionRingThickness = 5.f;
	};

	struct MineLayerPresentationProfile
	{
		sas::ContentId profileId;
		MineLayerVisualDefinition visual;
	};

	bool RegisterMineLayerPresentationProfiles();
}
