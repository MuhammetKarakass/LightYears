#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationIds.h"

namespace ly
{
	bool RegisterCrystalBarricadePresentationProfiles()
	{
		static const bool registered = []
		{
			CrystalBarricadeWallPresentationProfile profile;
			profile.profileId = CrystalBarricadePresentationIds::WallBasic;
			return PresentationProfileRegistry<CrystalBarricadeWallPresentationProfile>::Register(profile);
		}();
		return registered;
	}
}
