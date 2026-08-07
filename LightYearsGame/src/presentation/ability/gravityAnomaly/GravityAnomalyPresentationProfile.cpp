#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace ly
{
	namespace
	{
		GravityAnomalyProjectilePresentationProfile BuildProjectileProfile()
		{
			GravityAnomalyProjectilePresentationProfile profile;
			profile.profileId = GravityAnomalyPresentationIds::ProjectileBasic;
			profile.texturePath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			return profile;
		}

		GravityAnomalyFieldPresentationProfile BuildFieldProfile()
		{
			GravityAnomalyFieldPresentationProfile profile;
			profile.profileId = GravityAnomalyPresentationIds::FieldBasic;
			return profile;
		}
	}

	bool RegisterGravityAnomalyPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Register(
				BuildProjectileProfile()
			) &&
			PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Register(
				BuildFieldProfile()
			);
		return registered;
	}
}
