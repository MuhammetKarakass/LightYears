#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/nullPulse/NullPulsePresentationIds.h"

namespace ly
{
	bool RegisterNullPulsePresentationProfiles()
	{
		static const bool registered = []
		{
			NullPulsePresentationProfile profile;
			profile.profileId = NullPulsePresentationIds::PulseBasic;
			return PresentationProfileRegistry<NullPulsePresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
