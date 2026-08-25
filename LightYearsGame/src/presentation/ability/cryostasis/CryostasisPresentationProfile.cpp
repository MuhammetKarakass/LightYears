#include "presentation/ability/cryostasis/CryostasisPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/cryostasis/CryostasisPresentationIds.h"

namespace ly
{
	bool RegisterCryostasisPresentationProfiles()
	{
		static const bool registered = []
		{
			CryostasisPresentationProfile profile;
			profile.profileId = CryostasisPresentationIds::Basic;
			return PresentationProfileRegistry<CryostasisPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
