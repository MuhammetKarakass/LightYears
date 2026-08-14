#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationIds.h"

namespace ly
{
	bool RegisterOrbitalDronesPresentationProfiles()
	{
		static const bool registered = []
		{
			OrbitalDronesPresentationProfile profile;
			profile.profileId = OrbitalDronesPresentationIds::DroneBasic;
			profile.visual.glowColor = sf::Color{ 35, 190, 255, 125 };
			profile.visual.bodyColor = sf::Color{ 80, 220, 255, 235 };
			profile.visual.coreColor = sf::Color{ 225, 255, 255, 255 };
			profile.visual.tetherColor = sf::Color{ 65, 185, 255, 95 };
			profile.visual.trailColor = sf::Color{ 75, 210, 255, 105 };
			profile.visual.bodyRadius = 7.f;
			profile.visual.coreRadius = 2.5f;
			profile.visual.glowRadius = 17.f;
			profile.visual.pulseSpeed = 10.f;
			profile.visual.trailDuration = 0.12f;
			profile.visual.trailSegments = 6;
			profile.visual.expiryFadeDuration = 0.8f;
			return PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
