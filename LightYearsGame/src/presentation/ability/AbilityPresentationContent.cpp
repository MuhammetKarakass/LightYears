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
#include "presentation/ability/railBurst/RailBurstPresentationProfile.h"
#include "presentation/ability/crescentReaver/CrescentReaverPresentationProfile.h"
#include "presentation/ability/mineLayer/MineLayerPresentationProfile.h"
#include "presentation/ability/energySpear/EnergySpearPresentationProfile.h"
#include "presentation/ability/scorchDrive/ScorchDrivePresentationProfile.h"
#include "presentation/ability/ionStorm/IonStormPresentationProfile.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationProfile.h"
#include "presentation/ability/glacialPressure/GlacialPressurePresentationProfile.h"
#include "presentation/ability/chainLightning/ChainLightningPresentationProfile.h"
#include "presentation/ability/voidGate/VoidGatePresentationProfile.h"
#include "presentation/ability/cryostasis/CryostasisPresentationProfile.h"
#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationProfile.h"
#include "presentation/ability/frozenThrong/FrozenThrongPresentationProfile.h"
#include "presentation/ability/combatSentry/CombatSentryPresentationProfile.h"
#include "presentation/ability/nanoPlague/NanoPlaguePresentationProfile.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationProfile.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationProfile.h"
#include "presentation/ability/returnProtocol/ReturnProtocolPresentationProfile.h"
#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationProfile.h"

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
			RegisterRelayPrismPresentationProfiles() &&
			RegisterRailBurstPresentationProfiles() &&
			RegisterCrescentReaverPresentationProfiles() &&
			RegisterMineLayerPresentationProfiles() &&
			RegisterEnergySpearPresentationProfiles() &&
			RegisterScorchDrivePresentationProfiles() &&
			RegisterDirectionalBarrierPresentationProfiles() &&
			RegisterIonStormPresentationProfiles() &&
			RegisterGlacialPressurePresentationProfiles() &&
			RegisterChainLightningPresentationProfiles() &&
			RegisterCryostasisPresentationProfiles() &&
			RegisterVoidGatePresentationProfiles() &&
			RegisterFrostMaelstromPresentationProfiles() &&
			RegisterFrozenThrongPresentationProfiles() &&
			RegisterCombatSentryPresentationProfiles() &&
			RegisterNanoPlaguePresentationProfiles() &&
			RegisterAstralSurgePresentationProfiles() &&
			RegisterWingSentinelsPresentationProfiles() &&
			RegisterReturnProtocolPresentationProfiles() &&
			RegisterCrystalBarricadePresentationProfiles();
		return registered;
	}
}
