#include "presentation/ability/returnProtocol/ReturnProtocolPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/returnProtocol/ReturnProtocolPresentationIds.h"

namespace ly
{
	bool RegisterReturnProtocolPresentationProfiles()
	{
		static const bool registered = []
		{
			ReturnProtocolPresentationProfile profile;
			profile.profileId = ReturnProtocolPresentationIds::Basic;
			return PresentationProfileRegistry<ReturnProtocolPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
