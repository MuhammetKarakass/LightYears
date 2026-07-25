#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"

namespace ly
{
	namespace
	{
		SunBeamPresentationProfile BuildBasicSunBeamStrikeProfile()
		{
			SunBeamPresentationProfile profile;
			profile.profileId = SunBeamPresentationIds::StrikeBasic;

			profile.visual.outerColor = sf::Color{ 255, 165, 35, 150 };
			profile.visual.coreColor = sf::Color{ 255, 250, 220, 250 };
			profile.visual.groundGlowColor = sf::Color{ 255, 205, 90, 105 };
			profile.visual.impactRingColor = sf::Color{ 255, 245, 205, 230 };
			profile.visual.visibleLengthScale = 0.45f;
			profile.visual.arrivalStartWidthScale = 0.04f;
			profile.visual.arrivalEndWidthScale = 0.18f;
			profile.visual.impactWidthScale = 0.26f;
			profile.visual.groundGlowStartScale = 1.35f;
			profile.visual.groundGlowEndScale = 0.75f;
			profile.visual.impactRingEndScale = 1.9f;
			profile.visual.impactRingThickness = 5.f;
			profile.visual.pulseSpeed = 22.f;
			profile.visual.overheadStartScale = 0.12f;
			profile.visual.overheadEndScale = 1.f;
			profile.visual.convergenceHaloStartScale = 1.8f;
			profile.visual.convergenceHaloEndScale = 0.72f;
			profile.visual.impactFlashScale = 1.65f;
			profile.visual.impactGlareLengthScale = 2.4f;
			profile.visual.perspectiveSourceOffset = { -55.f, -520.f };
			profile.visual.impactColumnWidthScale = 1.8f;
			profile.visual.perspectiveCoreWidthScale = 0.32f;
			profile.visual.perspectiveShaftIntensity = 0.78f;
			profile.visual.impactFlashDuration = 0.07f;
			profile.visual.screenShakeAmplitude = 5.f;
			profile.visual.screenShakeDuration = 0.11f;
			profile.visual.screenShakeFrequency = 40.f;

			profile.telegraph.fillColor = sf::Color{ 235, 30, 30, 85 };
			profile.telegraph.outlineColor = sf::Color{ 255, 245, 245, 255 };
			profile.telegraph.outlineThickness = 4.f;
			profile.telegraph.pulseSpeed = 8.f;
			profile.telegraph.minimumPulse = 0.8f;
			profile.telegraph.maximumPulse = 1.f;
			profile.telegraph.pulseScaleAmount = 0.035f;
			profile.telegraph.dangerFillColor = sf::Color{ 255, 45, 20, 135 };
			profile.telegraph.dangerOutlineColor = sf::Color{ 255, 220, 110, 255 };
			profile.telegraph.countdownStartScale = 1.45f;
			profile.telegraph.countdownEndScale = 1.f;
			profile.telegraph.countdownEaseExponent = 2.6f;
			profile.telegraph.countdownRingThickness = 3.f;
			return profile;
		}
	}

	bool RegisterSunBeamPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<SunBeamPresentationProfile>::Register(
				BuildBasicSunBeamStrikeProfile()
			);
		return registered;
	}
}
