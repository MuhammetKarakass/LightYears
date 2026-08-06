#include "presentation/ability/AbilityPresentationContent.h"

#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"

namespace ly
{
	bool RegisterGameAbilityPresentationContent()
	{
		static const bool registered =
			RegisterGravityAnomalyPresentationProfiles() &&
			RegisterRocketPresentationProfiles() &&
			RegisterSunBeamPresentationProfiles() &&
			RegisterInfernoSprayPresentationProfiles();
		return registered;
	}
}
