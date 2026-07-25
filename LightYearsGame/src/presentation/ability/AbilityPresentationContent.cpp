#include "presentation/ability/AbilityPresentationContent.h"

#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"

namespace ly
{
	bool RegisterGameAbilityPresentationContent()
	{
		static const bool registered =
			RegisterRocketPresentationProfiles() &&
			RegisterSunBeamPresentationProfiles();
		return registered;
	}
}
