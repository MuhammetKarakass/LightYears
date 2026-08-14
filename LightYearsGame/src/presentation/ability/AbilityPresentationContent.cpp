#include "presentation/ability/AbilityPresentationContent.h"

#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationProfile.h"
#include "presentation/ability/nullPulse/NullPulsePresentationProfile.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationProfile.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationProfile.h"
#include "presentation/ability/hullShock/HullShockPresentationProfile.h"
#include "presentation/ability/shieldHarvest/ShieldHarvestPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"
#include "presentation/ability/relayPrism/RelayPrismPresentationProfile.h"

namespace ly
{
	bool RegisterGameAbilityPresentationContent()
	{
		static const bool registered =
			RegisterGravityAnomalyPresentationProfiles() &&
			RegisterRocketPresentationProfiles() &&
			RegisterOverdriveCorePresentationProfiles() &&
			RegisterNullPulsePresentationProfiles() &&
			RegisterOrbitalDronesPresentationProfiles() &&
			RegisterPhaseDriftPresentationProfiles() &&
			RegisterShieldHarvestPresentationProfiles() &&
			RegisterHullShockPresentationProfiles() &&
			RegisterSunBeamPresentationProfiles() &&
			RegisterInfernoSprayPresentationProfiles() &&
			RegisterRelayPrismPresentationProfiles();
		return registered;
	}
}
