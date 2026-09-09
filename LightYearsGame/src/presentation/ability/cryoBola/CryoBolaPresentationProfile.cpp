#include "presentation/ability/cryoBola/CryoBolaPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/cryoBola/CryoBolaPresentationIds.h"

namespace ly
{
	namespace
	{
		CryoBolaPresentationProfile BuildBasicCryoBolaProfile()
		{
			CryoBolaPresentationProfile profile;
			profile.profileId = CryoBolaPresentationIds::ProjectileBasic;
			return profile;
		}
	}

	bool RegisterCryoBolaPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<CryoBolaPresentationProfile>::Register(
				BuildBasicCryoBolaProfile()
			);
		return registered;
	}
}
