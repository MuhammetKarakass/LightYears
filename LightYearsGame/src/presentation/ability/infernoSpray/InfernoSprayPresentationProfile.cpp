#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace ly
{
	namespace
	{
		InfernoSprayPresentationProfile BuildBasicInfernoSprayProfile()
		{
			InfernoSprayPresentationProfile profile;
			profile.profileId = InfernoSprayPresentationIds::FlameConeBasic;
			profile.visual.outerFlameColor = sf::Color{ 255, 100, 20, 200 };
			profile.visual.coreFlameColor = sf::Color{ 255, 240, 120, 255 };
			profile.visual.flickerIntensity = 0.02f;
			profile.visual.pulseSpeed = 6.f;
			profile.visual.coreWidthRatio = 0.45f;
			return profile;
		}
	}

	bool RegisterInfernoSprayPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<InfernoSprayPresentationProfile>::Register(
				BuildBasicInfernoSprayProfile()
			);
		return registered;
	}
}
