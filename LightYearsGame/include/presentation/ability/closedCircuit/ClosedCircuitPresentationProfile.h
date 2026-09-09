#pragma once
#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>
namespace ly
{
	struct ClosedCircuitPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color deliveryColor{ 120, 230, 255, 245 };
		sf::Color formationColor{ 150, 245, 255, 210 };
		sf::Color fieldColor{ 100, 230, 255, 245 };
		sf::Color lowHealthColor{ 255, 115, 90, 230 };
		float fieldWidth = 7.f;
		float deliveryRadius = 9.f;
	};
	bool RegisterClosedCircuitPresentationProfiles();
}
