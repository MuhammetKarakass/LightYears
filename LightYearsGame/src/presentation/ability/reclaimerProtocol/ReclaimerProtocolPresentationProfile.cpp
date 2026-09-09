#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationIds.h"

namespace ly
{
	bool RegisterReclaimerProtocolPresentationProfiles()
	{
		static const bool registered = []
		{
			ReclaimerProtocolPresentationProfile profile;
			profile.profileId = ReclaimerProtocolPresentationIds::RepairKitBasic;
			return PresentationProfileRegistry<ReclaimerProtocolPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}