#include "presentation/ability/relayPrism/RelayPrismPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/relayPrism/RelayPrismPresentationIds.h"

namespace ly
{
	bool RegisterRelayPrismPresentationProfiles()
	{
		static const bool registered = []
		{
			RelayPrismPresentationProfile profile;
			profile.profileId = RelayPrismPresentationIds::RelayBasic;
			profile.relayTelegraph.drawInteriorFill = false;
			profile.relayTelegraph.drawCountdownRing = false;
			profile.relayTelegraph.outlineColor = sf::Color{ 220, 100, 255, 235 };
			profile.relayTelegraph.dangerOutlineColor = sf::Color{ 255, 205, 255, 255 };
			profile.relayTelegraph.outlineThickness = 3.f;
			profile.relayTelegraph.pulseSpeed = 5.f;
			profile.relayTelegraph.minimumPulse = 0.92f;
			profile.relayTelegraph.maximumPulse = 1.04f;
			profile.relayTelegraph.pulseScaleAmount = 0.025f;
			profile.relayTelegraph.countdownStartScale = 1.f;
			profile.relayTelegraph.countdownEndScale = 1.f;
			profile.relayTelegraph.completionFeedbackDuration = 0.18f;
			profile.relayTelegraph.completionOutlineColor = sf::Color{ 255, 245, 255, 255 };
			profile.relayTelegraph.completionStartScale = 1.f;
			profile.relayTelegraph.completionEndScale = 1.12f;
			return PresentationProfileRegistry<RelayPrismPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
