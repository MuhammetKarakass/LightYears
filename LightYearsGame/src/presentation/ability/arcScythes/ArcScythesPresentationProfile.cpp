#include "presentation/ability/arcScythes/ArcScythesPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/arcScythes/ArcScythesPresentationIds.h"

namespace ly
{
	bool RegisterArcScythesPresentationProfiles()
	{
		static const bool registered = []
		{
			ArcScythesPresentationProfile profile;
			profile.profileId = ArcScythesPresentationIds::BeamBasic;
			return PresentationProfileRegistry<ArcScythesPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
