#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationIds.h"

namespace ly
{
	bool RegisterFoldspaceArenaPresentationProfiles()
	{
		static const bool registered = []
		{
			FoldspaceArenaPresentationProfile profile;
			profile.profileId = FoldspaceArenaPresentationIds::ArenaBasic;
			return PresentationProfileRegistry<FoldspaceArenaPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
