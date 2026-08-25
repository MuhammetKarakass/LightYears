#include "presentation/ability/combatSentry/CombatSentryPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/combatSentry/CombatSentryPresentationIds.h"

namespace ly
{
	bool RegisterCombatSentryPresentationProfiles()
	{
		static const bool registered = []
		{
			CombatSentryTurretPresentationProfile turret;
			turret.profileId = CombatSentryPresentationIds::TurretBasic;
			CombatSentryProjectilePresentationProfile projectile;
			projectile.profileId = CombatSentryPresentationIds::ProjectileBasic;
			return PresentationProfileRegistry<CombatSentryTurretPresentationProfile>::Register(
				turret
			) && PresentationProfileRegistry<CombatSentryProjectilePresentationProfile>::Register(
				projectile
			);
		}();
		return registered;
	}
}
