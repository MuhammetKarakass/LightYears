#include "presentation/ability/AbilityPresentationContent.h"

#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationProfile.h"
#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"

namespace ly
{
	bool RegisterGameAbilityPresentationContent()
	{
		static const bool registered =
			RegisterGravityAnomalyPresentationProfiles() &&
			RegisterRocketPresentationProfiles() &&
			RegisterOverdriveCorePresentationProfiles() &&
			RegisterNullPulsePresentationProfiles() &&
			RegisterPhaseDriftPresentationProfiles() &&
			RegisterSunBeamPresentationProfiles() &&
			RegisterInfernoSprayPresentationProfiles();
		return registered;
	}
}
