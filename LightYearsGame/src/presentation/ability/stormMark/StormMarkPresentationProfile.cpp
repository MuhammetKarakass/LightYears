#include "presentation/ability/stormMark/StormMarkPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/stormMark/StormMarkPresentationIds.h"

namespace ly
{
	bool RegisterStormMarkPresentationProfiles()
	{
		static const bool registered = []
		{
			StormMarkPresentationProfile profile;
			profile.profileId = StormMarkPresentationIds::LightningBasic;
			return PresentationProfileRegistry<StormMarkPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
