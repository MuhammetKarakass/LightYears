#include "presentation/ability/crescentReaver/CrescentReaverPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/crescentReaver/CrescentReaverPresentationIds.h"

namespace ly
{
	namespace
	{
		CrescentReaverPresentationProfile BuildBasicCrescentReaverProfile()
		{
			CrescentReaverPresentationProfile profile;
			profile.profileId = CrescentReaverPresentationIds::ProjectileBasic;
			profile.visual.bladeColor = sf::Color{ 225, 240, 255, 245 };
			profile.visual.edgeColor = sf::Color{ 80, 185, 255, 230 };
			profile.visual.glowColor = sf::Color{ 35, 145, 255, 120 };
			profile.visual.impactColor = sf::Color{ 235, 250, 255, 245 };
			profile.visual.impactRingColor = sf::Color{ 75, 180, 255, 235 };
			profile.visual.radius = 24.f;
			profile.visual.bladeThickness = 8.f;
			profile.visual.glowRadius = 30.f;
			profile.visual.spinDegreesPerSecond = 720.f;
			profile.visual.trailLength = 34.f;
			profile.visual.trailWidth = 8.f;
			return profile;
		}
	}

	bool RegisterCrescentReaverPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<CrescentReaverPresentationProfile>::Register(
				BuildBasicCrescentReaverProfile()
			);
		return registered;
	}
}
