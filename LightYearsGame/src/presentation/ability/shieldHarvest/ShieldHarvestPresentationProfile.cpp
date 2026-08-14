#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationIds.h"

namespace ly
{
	bool RegisterShieldHarvestPresentationProfiles()
	{
		static const bool registered = []
		{
			ShieldHarvestPresentationProfile profile;
			profile.profileId = ShieldHarvestPresentationIds::FocusBasic;
			profile.focusTelegraph.drawInteriorFill = true;
			profile.focusTelegraph.fillMode = AreaTelegraphFillMode::RadialProgress;
			profile.focusTelegraph.drawCountdownRing = false;
			profile.focusTelegraph.fillColor = sf::Color{ 60, 150, 255, 72 };
			profile.focusTelegraph.outlineColor = sf::Color{ 95, 220, 255, 220 };
			profile.focusTelegraph.dangerFillColor = sf::Color{ 80, 210, 255, 120 };
			profile.focusTelegraph.dangerOutlineColor = sf::Color{ 220, 250, 255, 255 };
			profile.focusTelegraph.outlineThickness = 2.5f;
			profile.focusTelegraph.pulseSpeed = 4.5f;
			profile.focusTelegraph.minimumPulse = 0.72f;
			profile.focusTelegraph.maximumPulse = 1.f;
			profile.focusTelegraph.pulseScaleAmount = 0.018f;
			profile.focusTelegraph.countdownStartScale = 1.08f;
			profile.focusTelegraph.countdownEndScale = 1.f;
			profile.focusTelegraph.countdownEaseExponent = 1.f;
			// A mild normalized log-like curve makes the fill expand quickly
			// near the ship and slow down smoothly near the final radius.
			profile.focusTelegraph.radialGrowthLogStrength = 2.f;
			// 1.25 / 1.5 = 0.8333: 95% fills in the first phase and
			// the final 5% is reserved for the last 0.25 seconds.
			profile.focusTelegraph.radialGrowthPrimaryPhaseEnd = 0.833333f;
			profile.focusTelegraph.radialGrowthPrimaryPhaseFill = 0.95f;
			profile.focusTelegraph.completionFeedbackDuration = 0.35f;
			profile.focusTelegraph.completionFillColor = sf::Color{ 180, 240, 255, 210 };
			profile.focusTelegraph.completionOutlineColor = sf::Color{ 245, 255, 255, 255 };
			profile.focusTelegraph.completionStartScale = 1.f;
			profile.focusTelegraph.completionEndScale = 1.14f;
			profile.focusTelegraph.countdownRingThickness = 4.f;
			return PresentationProfileRegistry<ShieldHarvestPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
