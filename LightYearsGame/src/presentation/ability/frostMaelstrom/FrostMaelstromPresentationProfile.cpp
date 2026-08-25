#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationIds.h"

namespace ly
{
	bool RegisterFrostMaelstromPresentationProfiles()
	{
		static const bool registered = []
		{
			FrostMaelstromPresentationProfile profile;
			profile.profileId = FrostMaelstromPresentationIds::FieldBasic;
			return PresentationProfileRegistry<FrostMaelstromPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
