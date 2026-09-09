#include "presentation/ability/seismicCharge/SeismicChargePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/seismicCharge/SeismicChargePresentationIds.h"

namespace ly
{
	bool RegisterSeismicChargePresentationProfiles()
	{
		static const bool registered = []
		{
			SeismicChargePresentationProfile profile;
			profile.profileId = SeismicChargePresentationIds::BombBasic;
			return PresentationProfileRegistry<SeismicChargePresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
