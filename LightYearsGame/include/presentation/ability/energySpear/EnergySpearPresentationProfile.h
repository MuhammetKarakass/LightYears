#pragma once

#include "content/ContentId.h"
#include "presentation/ability/common/DirectionalChargeTelegraphVisualDefinition.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct EnergySpearTraversalVisualDefinition
	{
		sf::Color impactColor{ 235, 255, 255, 245 };
		sf::Color impactRingColor{ 70, 215, 255, 230 };
		float impactRadius = 22.f;
		float impactDuration = 0.12f;
		float impactRingThickness = 3.f;
	};

	struct EnergySpearPresentationProfile
	{
		sas::ContentId profileId;
		DirectionalChargeTelegraphVisualDefinition chargeTelegraph;
		EnergySpearTraversalVisualDefinition traversal;
	};

	bool RegisterEnergySpearPresentationProfiles();
}
