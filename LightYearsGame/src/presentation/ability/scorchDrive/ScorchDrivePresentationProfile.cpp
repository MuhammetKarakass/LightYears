#include "presentation/ability/scorchDrive/ScorchDrivePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/scorchDrive/ScorchDrivePresentationIds.h"

namespace ly
{
	namespace
	{
		ScorchDrivePresentationProfile BuildBasicScorchDriveProfile()
		{
			ScorchDrivePresentationProfile profile;
			profile.profileId = ScorchDrivePresentationIds::FireSegmentBasic;
			profile.visual.outerColor = sf::Color{ 255, 55, 8, 115 };
			profile.visual.coreColor = sf::Color{ 255, 205, 70, 220 };
			profile.visual.edgeColor = sf::Color{ 255, 115, 18, 215 };
			profile.visual.edgeThickness = 3.f;
			profile.visual.pulseSpeed = 8.f;
			profile.visual.fadeExponent = 1.35f;
			return profile;
		}
	}

	bool RegisterScorchDrivePresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<ScorchDrivePresentationProfile>::Register(
				BuildBasicScorchDriveProfile()
			);
		return registered;
	}
}
