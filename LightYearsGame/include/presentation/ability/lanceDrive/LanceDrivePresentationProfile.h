#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct LanceDriveVisualDefinition
	{
		sf::Color edgeColor{ 255, 174, 76, 235 };
		float edgeThickness = 5.f;
		float pulseSpeed = 8.f;
	};

	struct LanceDrivePresentationProfile
	{
		sas::ContentId profileId;
		LanceDriveVisualDefinition visual;
	};

	bool RegisterLanceDrivePresentationProfiles();
}
