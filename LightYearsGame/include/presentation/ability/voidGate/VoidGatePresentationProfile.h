#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct VoidGatePortalVisualDefinition
	{
		sf::Color outerColor{ 100, 40, 230, 235 };
		sf::Color innerColor{ 30, 10, 100, 215 };
		sf::Color glowColor{ 180, 100, 255, 150 };
		float outerThickness = 5.f;
		float innerThickness = 2.f;
		float pulseSpeed = 5.f;
		float pulseAmount = 0.06f;
	};

	struct VoidGatePresentationProfile
	{
		sas::ContentId profileId;
		VoidGatePortalVisualDefinition portal;
	};

	bool RegisterVoidGatePresentationProfiles();
}
