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
				sf::Color{ 70, 185, 255, 145 },
				sf::Color{ 220, 250, 255, 240 },
				sf::Color{ 105, 210, 255, 220 },
				25.f,
				0.18f,
				0.34f
			}
		);
	}
}
