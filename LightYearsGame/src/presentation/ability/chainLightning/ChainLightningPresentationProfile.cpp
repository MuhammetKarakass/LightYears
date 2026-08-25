#include "presentation/ability/chainLightning/ChainLightningPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationIds.h"

namespace ly
{
	bool RegisterChainLightningPresentationProfiles()
	{
		static const bool registered = []
		{
			ChainLightningPresentationProfile profile;
			profile.profileId = ChainLightningPresentationIds::ArcBasic;
			return PresentationProfileRegistry<ChainLightningPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
