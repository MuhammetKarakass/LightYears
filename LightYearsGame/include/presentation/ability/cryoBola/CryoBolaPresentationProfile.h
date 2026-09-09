#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	// Kept family-local: the linked spinning weights and the rupture are unique
	// to Cryo Bola, so they do not enlarge a global visual configuration type.
	struct CryoBolaVisualDefinition
	{
		sf::Color orbColor{ 105, 225, 255, 245 };
		sf::Color orbGlowColor{ 80, 165, 255, 115 };
		sf::Color tetherColor{ 210, 250, 255, 220 };
		sf::Color ruptureColor{ 130, 230, 255, 200 };
		sf::Color ruptureLineColor{ 235, 255, 255, 225 };
		float orbRadius = 10.f;
		float orbOrbitRadius = 13.f;
		float spinRadiansPerSecond = 18.f;
		float ruptureDuration = 0.32f;
		float ruptureVisualRadius = 500.f;
		float ruptureOutlineThickness = 3.f;
		int ruptureLineCount = 10;
	};

	struct CryoBolaPresentationProfile
	{
		sas::ContentId profileId;
		CryoBolaVisualDefinition visual;
	};

	bool RegisterCryoBolaPresentationProfiles();
}
