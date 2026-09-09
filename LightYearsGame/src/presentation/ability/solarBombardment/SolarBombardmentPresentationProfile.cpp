#include "presentation/ability/solarBombardment/SolarBombardmentPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/solarBombardment/SolarBombardmentPresentationIds.h"

namespace ly
{
	namespace
	{
		SolarBombardmentPresentationProfile BuildBasicProfile()
		{
			SolarBombardmentPresentationProfile profile;
			profile.profileId = SolarBombardmentPresentationIds::ProjectileBasic;

			// The delivery projectile is deliberately warm/orange, while the
			// impact keeps a pale center so its two damage zones read separately.
			profile.projectile.outerColor = sf::Color{ 255, 115, 25, 220 };
			profile.projectile.coreColor = sf::Color{ 255, 238, 170, 255 };
			profile.projectile.trailColor = sf::Color{ 255, 90, 20, 155 };
			profile.projectile.radius = 14.f;
			profile.projectile.trailLength = 70.f;

			profile.explosion.outerRadius = 600.f;
			profile.explosion.innerRadius = 300.f;
			profile.explosion.duration = 0.32f;

			profile.outerTelegraph.drawInteriorFill = false;
			profile.outerTelegraph.drawCountdownRing = true;
			profile.outerTelegraph.outlineColor = sf::Color{ 255, 125, 35, 235 };
			profile.outerTelegraph.dangerOutlineColor = sf::Color{ 255, 235, 160, 255 };
			profile.outerTelegraph.outlineThickness = 3.f;
			profile.outerTelegraph.countdownRingThickness = 4.f;
			profile.outerTelegraph.countdownStartScale = 1.f;
			profile.outerTelegraph.countdownEndScale = 1.f;
			profile.outerTelegraph.completionFeedbackDuration = 0.12f;

			profile.innerTelegraph = profile.outerTelegraph;
			profile.innerTelegraph.outlineColor = sf::Color{ 255, 235, 120, 245 };
			profile.innerTelegraph.dangerOutlineColor = sf::Color{ 255, 255, 225, 255 };
			return profile;
		}
	}

	bool RegisterSolarBombardmentPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<SolarBombardmentPresentationProfile>::Register(
				BuildBasicProfile()
			);
		return registered;
	}
}
