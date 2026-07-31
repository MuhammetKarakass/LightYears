#include "presentation/ability/infernoSpray/InfernoSprayPresentationIds.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/PresentationProfileRegistry.h"

namespace ly
{
	namespace
	{
		InfernoSprayPresentationProfile BuildBasicInfernoSprayProfile()
		{
			InfernoSprayPresentationProfile profile;
			profile.profileId = InfernoSprayPresentationIds::Basic;

			profile.visual.outerFlameColor = sf::Color{ 255, 85, 15, 210 };
			profile.visual.coreFlameColor = sf::Color{ 255, 240, 140, 255 };
			profile.visual.smokeColor = sf::Color{ 50, 45, 45, 170 };
			profile.visual.flameRange = 520.f;
			profile.visual.coneAngleDegrees = 36.f;
			profile.visual.coreWidthRatio = 0.35f;
			profile.visual.pulseSpeed = 20.f;
			profile.visual.flickerIntensity = 0.18f;
			profile.visual.screenShakeAmplitude = 1.8f;
			profile.visual.screenShakeFrequency = 28.f;

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