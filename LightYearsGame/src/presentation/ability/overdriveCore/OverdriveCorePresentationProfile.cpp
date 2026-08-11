#include "presentation/ability/overdriveCore/OverdriveCorePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationIds.h"

namespace ly
{
	namespace
	{
		OverdriveCorePresentationProfile BuildBasicOverdriveCoreProfile()
		{
			OverdriveCorePresentationProfile profile;
			profile.profileId = OverdriveCorePresentationIds::ProjectileBasic;
			profile.texturePath = "SpaceShooterRedux/PNG/Lasers/laserRed01.png";
			profile.visual.bodyColor = sf::Color{ 255, 105, 30, 245 };
			profile.visual.coreColor = sf::Color{ 255, 245, 170, 255 };
			profile.visual.trailColor = sf::Color{ 255, 55, 15, 170 };
			profile.visual.impactColor = sf::Color{ 255, 180, 65, 245 };
			profile.visual.bodyRadius = 4.f;
			profile.visual.trailLength = 26.f;
			profile.visual.trailWidth = 8.f;
			profile.visual.glowRadius = 10.f;
			profile.visual.pulseSpeed = 34.f;
			profile.visual.impactVisualDuration = 0.24f;
			profile.visual.impactRingEndScale = 1.7f;
			profile.visual.impactRingThickness = 3.f;
			profile.visual.screenShakeAmplitude = 1.5f;
			profile.visual.screenShakeDuration = 0.06f;
			profile.visual.screenShakeFrequency = 46.f;
			profile.telegraph.fillColor = sf::Color{ 255, 75, 25, 28 };
			profile.telegraph.outlineColor = sf::Color{ 255, 130, 55, 155 };
			profile.telegraph.outlineThickness = 2.f;
			profile.telegraph.pulseSpeed = 12.f;
			profile.telegraph.minimumPulse = 0.72f;
			profile.telegraph.maximumPulse = 1.f;
			profile.telegraph.countdownStartScale = 1.15f;
			profile.telegraph.countdownEndScale = 1.f;
			profile.telegraph.countdownRingThickness = 2.f;
			profile.explosionType = ExplosionType::Small;
			return profile;
		}
	}

	bool RegisterOverdriveCorePresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<OverdriveCorePresentationProfile>::Register(
				BuildBasicOverdriveCoreProfile()
			);
		return registered;
	}
}
