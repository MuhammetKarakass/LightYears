#include "presentation/ability/aegisReaver/AegisReaverPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/aegisReaver/AegisReaverPresentationIds.h"

namespace ly
{
	namespace
	{
		AegisReaverPresentationProfile BuildBasicAegisReaverProfile()
		{
			AegisReaverPresentationProfile profile;
			profile.profileId = AegisReaverPresentationIds::ProjectileBasic;
			return profile;
		}
	}

	bool RegisterAegisReaverPresentationProfiles()
	{
		static const bool registered =
			PresentationProfileRegistry<AegisReaverPresentationProfile>::Register(
				BuildBasicAegisReaverProfile()
			);
		return registered;
	}
}
