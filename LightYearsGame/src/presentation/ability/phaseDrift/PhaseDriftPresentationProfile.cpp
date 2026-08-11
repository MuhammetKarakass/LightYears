#include "presentation/ability/phaseDrift/PhaseDriftPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationIds.h"

namespace ly
{
	bool RegisterPhaseDriftPresentationProfiles()
	{
		static const bool registered = []
		{
			PhaseDriftPresentationProfile profile;
			profile.profileId = PhaseDriftPresentationIds::Basic;
			profile.auraColor = sf::Color{ 95, 205, 255, 72 };
			profile.outlineColor = sf::Color{ 165, 235, 255, 105 };
			profile.radius = 52.f;
			profile.outlineThickness = 2.5f;
			profile.pulseSpeed = 6.f;
			return PresentationProfileRegistry<PhaseDriftPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
