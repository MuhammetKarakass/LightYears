#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationIds.h"

namespace ly
{
	bool RegisterTemporalConvergencePresentationProfiles()
	{
		static const bool registered = []
		{
			TemporalConvergencePresentationProfile profile;
			profile.profileId = TemporalConvergencePresentationIds::FieldBasic;
			profile.targetTelegraph.drawInteriorFill = true;
			profile.targetTelegraph.fillMode = AreaTelegraphFillMode::Static;
			// One exact-radius outline is enough. The generic countdown ring and its
			// enlarged start scale would otherwise create a misleading second circle.
			profile.targetTelegraph.drawCountdownRing = false;
			profile.targetTelegraph.fillColor = sf::Color{ 84, 92, 255, 34 };
			profile.targetTelegraph.outlineColor = sf::Color{ 125, 195, 255, 205 };
			profile.targetTelegraph.dangerFillColor = sf::Color{ 125, 210, 255, 92 };
			profile.targetTelegraph.dangerOutlineColor = sf::Color{ 225, 245, 255, 255 };
			profile.targetTelegraph.outlineThickness = 2.5f;
			profile.targetTelegraph.pulseSpeed = 4.f;
			profile.targetTelegraph.minimumPulse = 0.82f;
			profile.targetTelegraph.maximumPulse = 1.f;
			profile.targetTelegraph.pulseScaleAmount = 0.f;
			profile.targetTelegraph.countdownStartScale = 1.f;
			profile.targetTelegraph.countdownEndScale = 1.f;
			profile.targetTelegraph.completionFeedbackDuration = 0.5f;
			profile.targetTelegraph.completionFillColor = sf::Color{ 175, 235, 255, 210 };
			profile.targetTelegraph.completionOutlineColor = sf::Color{ 245, 255, 255, 255 };
			// Entry feedback flashes the already-established full circle in place.
			// Equal scales intentionally prevent a center-out explosion animation.
			profile.targetTelegraph.completionStartScale = 1.f;
			profile.targetTelegraph.completionEndScale = 1.f;
			return PresentationProfileRegistry<
				TemporalConvergencePresentationProfile
			>::Register(profile);
		}();
		return registered;
	}
}
