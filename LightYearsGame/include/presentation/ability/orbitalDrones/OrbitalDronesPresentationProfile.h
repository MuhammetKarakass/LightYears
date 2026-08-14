#pragma once

#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>

namespace ly
{
	struct OrbitalDroneVisualDefinition
	{
		// The drone is drawn procedurally, so this feature does not require a
		// texture asset and can evolve independently from projectile visuals.
		sf::Color glowColor{ 35, 190, 255, 125 };
		sf::Color bodyColor{ 80, 220, 255, 235 };
		sf::Color coreColor{ 225, 255, 255, 255 };
		sf::Color tetherColor{ 65, 185, 255, 95 };
		sf::Color trailColor{ 75, 210, 255, 105 };
		float bodyRadius = 7.f;
		float coreRadius = 2.5f;
		float glowRadius = 17.f;
		float pulseSpeed = 10.f;
		float trailDuration = 0.12f;
		int trailSegments = 6;
		// The actor fades during the last part of its configured lifetime. The
		// lifetime itself remains owned by AbilityWorldActor/runtime setup.
		float expiryFadeDuration = 0.8f;
	};

	struct OrbitalDronesPresentationProfile
	{
		sas::ContentId profileId;
		OrbitalDroneVisualDefinition visual;
	};

	bool RegisterOrbitalDronesPresentationProfiles();
}
