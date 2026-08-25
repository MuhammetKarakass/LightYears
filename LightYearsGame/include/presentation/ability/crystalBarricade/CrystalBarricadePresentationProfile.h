#pragma once

#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct CrystalBarricadeWallPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color coreColor{ 90, 220, 255, 210 };
		sf::Color edgeColor{ 210, 250, 255, 255 };
		sf::Color breakColor{ 150, 235, 255, 230 };
		float breakVisualDuration = 0.30f;
	};

	bool RegisterCrystalBarricadePresentationProfiles();
}
