#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct IonStormProjectileVisualDefinition
	{
		sf::Color outerColor{ 70, 160, 255, 210 };
		sf::Color coreColor{ 190, 240, 255, 245 };
		sf::Color trailColor{ 80, 170, 255, 150 };
		float radius = 12.f;
		float trailLength = 46.f;
		float pulseSpeed = 12.f;
	};

	struct IonStormProjectilePresentationProfile
	{
		sas::ContentId profileId;
		IonStormProjectileVisualDefinition visual;
	};

	struct IonStormFieldVisualDefinition
	{
		// Ion Storm is intentionally rendered as one filled irregular shape.
		sf::Color fillColor{ 50, 130, 255, 85 };
		float pulseSpeed = 7.f;
		int renderPointCount = 64;
	};

	struct IonStormFieldPresentationProfile
	{
		sas::ContentId profileId;
		IonStormFieldVisualDefinition visual;
	};

	bool RegisterIonStormPresentationProfiles();
}
