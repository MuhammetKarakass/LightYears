#include "presentation/ability/nanoPlague/NanoPlaguePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/nanoPlague/NanoPlaguePresentationIds.h"

namespace ly
{
	bool RegisterNanoPlaguePresentationProfiles()
	{
		return PresentationProfileRegistry<NanoPlaguePresentationProfile>::Register(
			NanoPlaguePresentationProfile{
				sas::ContentId{ NanoPlaguePresentationIds::InfectionBasic },
				sf::Color{ 95, 255, 140, 125 },
				sf::Color{ 175, 255, 205, 230 },
				sf::Color{ 95, 255, 140, 200 },
				25.f,
				0.18f,
				0.34f
			}
		);
	}
}
