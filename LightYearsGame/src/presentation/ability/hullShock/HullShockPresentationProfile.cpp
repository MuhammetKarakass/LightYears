#include "presentation/ability/hullShock/HullShockPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/hullShock/HullShockPresentationIds.h"

namespace ly
{
	bool RegisterHullShockPresentationProfiles()
	{
		static const bool registered = []
		{
			HullShockPresentationProfile profile;
			profile.profileId = HullShockPresentationIds::FocusBasic;
			profile.focusTelegraph.drawInteriorFill = true;
			profile.focusTelegraph.fillMode = AreaTelegraphFillMode::RadialProgress;
			profile.focusTelegraph.minimumFillRadiusRatio = 0.5f;
			profile.focusTelegraph.drawCountdownRing = false;
			profile.focusTelegraph.fillColor = sf::Color{ 30, 120, 255, 60 };
			profile.focusTelegraph.outlineColor = sf::Color{ 75, 210, 255, 225 };
			profile.focusTelegraph.dangerFillColor = sf::Color{ 110, 245, 255, 145 };
			profile.focusTelegraph.dangerOutlineColor = sf::Color{ 235, 255, 255, 255 };
			profile.focusTelegraph.outlineThickness = 2.5f;
			profile.focusTelegraph.pulseSpeed = 7.f;
			profile.focusTelegraph.minimumPulse = 0.65f;
			profile.focusTelegraph.maximumPulse = 1.f;
			profile.focusTelegraph.pulseScaleAmount = 0.022f;
			profile.focusTelegraph.countdownStartScale = 1.f;
			profile.focusTelegraph.countdownEndScale = 1.f;
			profile.focusTelegraph.countdownEaseExponent = 1.f;
			profile.focusTelegraph.radialGrowthLogStrength = 2.f;
			// 1.25 / 2.0 = 0.625: the area becomes full before the
			// final damage-only charge phase.
			profile.focusTelegraph.radialGrowthPrimaryPhaseEnd = 0.625f;
			profile.focusTelegraph.radialGrowthPrimaryPhaseFill = 1.f;
			profile.focusTelegraph.completionFeedbackDuration = 0.30f;
			profile.focusTelegraph.completionFillColor = sf::Color{ 150, 245, 255, 220 };
			profile.focusTelegraph.completionOutlineColor = sf::Color{ 245, 255, 255, 255 };
			profile.focusTelegraph.completionStartScale = 1.f;
			profile.focusTelegraph.completionEndScale = 1.18f;
			profile.focusTelegraph.countdownRingThickness = 4.f;
			return PresentationProfileRegistry<HullShockPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
