#include "presentation/ability/astralSurge/AstralSurgePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"

namespace ly
{
	namespace
	{
		AstralSurgePresentationProfile BuildBasicAstralSurgeProfile()
		{
			AstralSurgePresentationProfile profile;
			profile.profileId = AstralSurgePresentationIds::ProjectileBasic;
			profile.projectile.outerColor = sf::Color{ 130, 70, 255, 185 };
			profile.projectile.coreColor = sf::Color{ 245, 235, 255, 255 };
			profile.projectile.glowColor = sf::Color{ 160, 105, 255, 130 };
			profile.projectile.impactColor = sf::Color{ 240, 220, 255, 230 };
			profile.projectile.crescentRadius = 140.f;
			profile.projectile.crescentThickness = 34.f;
			profile.projectile.crescentArcDegrees = 160.f;
			profile.projectile.glowThickness = 58.f;
			profile.projectile.pulseSpeed = 12.f;
			profile.projectile.impactRadius = 34.f;
			profile.projectile.impactDuration = 0.10f;
			return profile;
		}
	}

	bool RegisterAstralSurgePresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<AstralSurgePresentationProfile>::Register(
				BuildBasicAstralSurgeProfile()
			);
		return registered;
	}
}
