#include "presentation/ability/emberSwarm/EmberSwarmPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationIds.h"

namespace ly
{
	bool RegisterEmberSwarmPresentationProfiles()
	{
		static const bool registered = []
		{
			EmberSwarmPresentationProfile profile;
			profile.profileId = EmberSwarmPresentationIds::DroneBasic;
			profile.visual.glowColor = sf::Color{ 255, 120, 30, 140 };
			profile.visual.bodyColor = sf::Color{ 255, 180, 50, 240 };
			profile.visual.coreColor = sf::Color{ 255, 240, 200, 255 };
			profile.visual.bodyRadius = 6.f;
			profile.visual.coreRadius = 2.2f;
			profile.visual.glowRadius = 15.f;
			profile.visual.expiryFadeDuration = 0.6f;
			profile.visual.pulseColor = sf::Color{ 255, 80, 20, 200 };
			profile.visual.impactColor = sf::Color{ 240, 40, 10, 220 };
			profile.visual.pulseRadius = 18.f;
			profile.visual.pulseDuration = 0.12f;
			profile.visual.impactRadius = 10.f;
			profile.visual.impactDuration = 0.12f;
			profile.visual.pulseOutlineThickness = 1.2f;
			return PresentationProfileRegistry<EmberSwarmPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
