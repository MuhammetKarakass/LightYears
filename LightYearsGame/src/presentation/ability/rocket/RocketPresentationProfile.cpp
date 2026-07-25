#include "presentation/ability/rocket/RocketPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"

namespace ly
{
	namespace
	{
		RocketPresentationProfile BuildBasicRocketProfile()
		{
			RocketPresentationProfile profile;
			profile.profileId = RocketPresentationIds::Basic;

			profile.visual.exhaustOuterColor = sf::Color{ 255, 80, 25, 185 };
			profile.visual.exhaustCoreColor = sf::Color{ 255, 235, 150, 250 };
			profile.visual.flightGlowColor = sf::Color{ 255, 115, 45, 135 };
			profile.visual.impactFlashColor = sf::Color{ 255, 240, 190, 250 };
			profile.visual.impactRingColor = sf::Color{ 255, 105, 35, 240 };
			profile.visual.trailLength = 62.f;
			profile.visual.trailWidth = 18.f;
			profile.visual.coreTrailWidthScale = 0.34f;
			profile.visual.flightGlowRadius = 19.f;
			profile.visual.pulseSpeed = 26.f;
			profile.visual.impactVisualDuration = 0.36f;
			profile.visual.impactFlashEndScale = 1.4f;
			profile.visual.impactRingEndScale = 2.f;
			profile.visual.impactRingThickness = 5.f;
			profile.visual.screenShakeAmplitude = 3.5f;
			profile.visual.screenShakeDuration = 0.1f;
			profile.visual.screenShakeFrequency = 38.f;

			profile.telegraph.fillColor = sf::Color{ 255, 95, 35, 42 };
			profile.telegraph.outlineColor = sf::Color{ 255, 145, 70, 205 };
			profile.telegraph.outlineThickness = 3.f;
			profile.telegraph.pulseSpeed = 10.f;
			profile.telegraph.minimumPulse = 0.65f;
			profile.telegraph.maximumPulse = 1.f;
			profile.telegraph.pulseScaleAmount = 0.045f;
			profile.telegraph.dangerFillColor = sf::Color{ 255, 45, 20, 105 };
			profile.telegraph.dangerOutlineColor = sf::Color{ 255, 225, 135, 255 };
			profile.telegraph.countdownStartScale = 1.3f;
			profile.telegraph.countdownEndScale = 1.f;
			profile.telegraph.countdownEaseExponent = 1.8f;
			profile.telegraph.countdownRingThickness = 4.f;

			profile.explosionType = ExplosionType::Small;
			return profile;
		}
	}

	bool RegisterRocketPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<RocketPresentationProfile>::Register(
				BuildBasicRocketProfile()
			);
		return registered;
	}
}
