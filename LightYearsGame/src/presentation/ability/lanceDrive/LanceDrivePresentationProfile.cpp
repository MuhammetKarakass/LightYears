#include "presentation/ability/lanceDrive/LanceDrivePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationIds.h"

namespace ly
{
	namespace
	{
		LanceDrivePresentationProfile BuildBasicProfile()
		{
			LanceDrivePresentationProfile profile;
			profile.profileId = LanceDrivePresentationIds::LanceBasic;
			profile.visual.edgeColor = sf::Color{ 255, 174, 76, 235 };
			profile.visual.edgeThickness = 5.f;
			profile.visual.pulseSpeed = 8.f;
			return profile;
		}
	}

	bool RegisterLanceDrivePresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<LanceDrivePresentationProfile>::Register(
				BuildBasicProfile()
			);
		return registered;
	}
}
