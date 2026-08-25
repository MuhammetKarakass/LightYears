#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct ScorchDriveVisualDefinition
	{
		sf::Color outerColor{ 255, 65, 10, 115 };
		sf::Color coreColor{ 255, 205, 70, 220 };
		sf::Color edgeColor{ 255, 120, 20, 210 };
		float edgeThickness = 3.f;
		float pulseSpeed = 8.f;
		float fadeExponent = 1.35f;
	};

	struct ScorchDrivePresentationProfile
	{
		sas::ContentId profileId;
		ScorchDriveVisualDefinition visual;
	};

	bool RegisterScorchDrivePresentationProfiles();
}
