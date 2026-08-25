#include "presentation/ability/glacialPressure/GlacialPressurePresentationProfile.h"

#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/glacialPressure/GlacialPressurePresentationIds.h"

namespace ly
{
	bool RegisterGlacialPressurePresentationProfiles()
	{
		static const bool registered = []
		{
			GlacialPressurePresentationProfile profile;
			profile.profileId = GlacialPressurePresentationIds::FocusBasic;
			return PresentationProfileRegistry<GlacialPressurePresentationProfile>::Register(
				profile
			);
		}();
		return registered;
	}
}
