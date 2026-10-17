#include "presentation/ability/frozenThrong/FrozenThrongPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/frozenThrong/FrozenThrongPresentationIds.h"

namespace vega
{
	bool RegisterFrozenThrongPresentationProfiles()
	{
		static const bool registered = []
		{
			FrozenThrongPresentationProfile profile;
			profile.profileId = FrozenThrongPresentationIds::HuskBasic;
			return PresentationProfileRegistry<FrozenThrongPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
