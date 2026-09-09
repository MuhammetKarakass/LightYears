#include "presentation/ability/blastback/BlastbackPresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/blastback/BlastbackPresentationIds.h"

namespace ly
{
	bool RegisterBlastbackPresentationProfiles()
	{
		static const bool registered = []
		{
			BlastbackPresentationProfile profile;
			profile.profileId = BlastbackPresentationIds::Basic;
			profile.emitterInnerRadius = 48.f;
			profile.focusExpansionDuration = 0.4f;
			return PresentationProfileRegistry<BlastbackPresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
