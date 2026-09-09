#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct InertialWakeVisualDefinition
	{
		sf::Color outerColor{ 110, 220, 255, 105 };
		sf::Color coreColor{ 220, 250, 255, 175 };
		sf::Color edgeColor{ 125, 235, 255, 230 };
		float edgeThickness = 2.5f;
		float pulseSpeed = 7.f;
	};

	struct InertialWakePresentationProfile
	{
		sas::ContentId profileId;
		InertialWakeVisualDefinition visual;
	};

	bool RegisterInertialWakePresentationProfiles();
}
