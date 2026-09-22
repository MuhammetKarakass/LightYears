#include "presentation/ability/stormMark/StormMarkPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/stormMark/StormMarkPresentationIds.h"

namespace ly
{
	bool RegisterStormMarkPresentationProfiles()
	{
		static const bool registered = []
		{
			StormMarkPresentationProfile profile;
			profile.profileId = StormMarkPresentationIds::LightningBasic;
			// A plain, static indicator: a dim outline with a barely-there fill that just
			// shows which space the mark covers. It must not read as light or as a warning.
			profile.searchArea.drawInteriorFill = true;
			profile.searchArea.fillMode = AreaTelegraphFillMode::Static;
			profile.searchArea.fillColor = sf::Color{ 65, 190, 255, 20 };
			profile.searchArea.outlineColor = sf::Color{ 65, 190, 255, 140 };
			profile.searchArea.outlineThickness = 2.f;
			// No closing outer-to-inner ring: the shared countdown ring is the thing that
			// animates inward, so it stays off and its scales stay neutral.
			profile.searchArea.drawCountdownRing = false;
			profile.searchArea.countdownRingThickness = 0.f;
			profile.searchArea.countdownStartScale = 1.f;
			profile.searchArea.countdownEndScale = 1.f;
			// No pulsing motion either.
			profile.searchArea.pulseSpeed = 0.f;
			profile.searchArea.minimumPulse = 1.f;
			profile.searchArea.maximumPulse = 1.f;
			profile.searchArea.pulseScaleAmount = 0.f;
			// The shared resolver blends toward the danger colours as the timer runs down;
			// pinning them to the base colours keeps the indicator from heating up.
			profile.searchArea.dangerFillColor = profile.searchArea.fillColor;
			profile.searchArea.dangerOutlineColor = profile.searchArea.outlineColor;
			return PresentationProfileRegistry<StormMarkPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
