#include "presentation/ability/wingSentinels/WingSentinelsPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationIds.h"

namespace ly
{
	bool RegisterWingSentinelsPresentationProfiles()
	{
		static const bool registered = []
		{
			WingSentinelsPresentationProfile profile;
			profile.profileId = WingSentinelsPresentationIds::Basic;
			return PresentationProfileRegistry<WingSentinelsPresentationProfile>::Register(profile);
		}();
		return registered;
	}
}
