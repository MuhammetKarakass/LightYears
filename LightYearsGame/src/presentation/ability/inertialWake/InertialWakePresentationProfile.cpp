#include "presentation/ability/inertialWake/InertialWakePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/inertialWake/InertialWakePresentationIds.h"

namespace ly
{
	namespace
	{
		InertialWakePresentationProfile BuildBasicInertialWakeProfile()
		{
			InertialWakePresentationProfile profile;
			profile.profileId = InertialWakePresentationIds::WakeBasic;
			profile.visual.outerColor = sf::Color{ 90, 205, 255, 95 };
			profile.visual.coreColor = sf::Color{ 215, 248, 255, 165 };
			profile.visual.edgeColor = sf::Color{ 120, 230, 255, 225 };
			profile.visual.edgeThickness = 2.5f;
			profile.visual.pulseSpeed = 7.f;
			return profile;
		}
	}

	bool RegisterInertialWakePresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<InertialWakePresentationProfile>::Register(
				BuildBasicInertialWakeProfile()
			);
		return registered;
	}
}
