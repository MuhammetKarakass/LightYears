#pragma once
#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>

namespace vega
{
	namespace IonCometPresentationIds
	{
		inline const sas::ContentId Basic{ "Presentation.Ability.IonComet.Field.Basic" };
	}
	struct IonCometPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color fieldFill{ 65, 160, 255, 14 };
		sf::Color fieldEdge{ 100, 205, 255, 155 };
		sf::Color burstEdge{ 180, 240, 255, 240 };
		float fieldThickness = 2.f;
		float burstThickness = 5.f;
		float burstSeconds = .25f;
	};
	bool RegisterIonCometPresentationProfiles();
}
