#include "gameplay/ability/content/GameAbilityContentRegistration.h"

#include "framework/Core.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/dash/DashAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayAbility.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreAbility.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreProjectileActor.h"
#include "gameplay/ability/nullPulse/NullPulseAbility.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesAbility.h"
#include "gameplay/ability/executionDrive/ExecutionDriveAbility.h"
#include "gameplay/ability/phaseDrift/PhaseDriftAbility.h"
#include "gameplay/ability/hullShock/HullShockAbility.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestAbility.h"
#include "gameplay/ability/rocket/RocketAbility.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/relayPrism/RelayPrismAbility.h"
#include "gameplay/ability/relayPrism/RelayPrismActor.h"
#include "gameplay/ability/echoProtocol/EchoProtocolAbility.h"
#include "gameplay/ability/railBurst/RailBurstAbility.h"
#include "gameplay/ability/railBurst/RailBurstProjectileActor.h"
#include "gameplay/ability/crescentReaver/CrescentReaverAbility.h"
#include "gameplay/ability/crescentReaver/CrescentReaverProjectileActor.h"
#include "gameplay/ability/aegisReaver/AegisReaverAbility.h"
#include "gameplay/ability/aegisReaver/AegisReaverProjectileActor.h"
#include "gameplay/ability/mineLayer/MineLayerAbility.h"
#include "gameplay/ability/mineLayer/MineLayerMineActor.h"
#include "gameplay/ability/energySpear/EnergySpearAbility.h"
#include "gameplay/ability/scorchDrive/ScorchDriveAbility.h"
#include "gameplay/ability/scorchDrive/ScorchDriveFireSegmentActor.h"
#include "gameplay/ability/ionStorm/IonStormAbility.h"
#include "gameplay/ability/ionStorm/IonStormFieldActor.h"
#include "gameplay/ability/ionStorm/IonStormProjectileActor.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentAbility.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentProjectileActor.h"
#include "gameplay/ability/strikeRun/StrikeRunAbility.h"
#include "gameplay/ability/blastback/BlastbackAbility.h"
#include "gameplay/ability/seismicCharge/SeismicChargeAbility.h"
#include "gameplay/ability/seismicCharge/SeismicChargeBombActor.h"
#include "gameplay/ability/arcScythes/ArcScythesAbility.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolAbility.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitAbility.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"
#include "gameplay/ability/vectorSync/VectorSyncAbility.h"
#include "gameplay/ability/zeroDrag/ZeroDragAbility.h"
#include "gameplay/ability/inertialWake/InertialWakeAbility.h"
#include "gameplay/ability/shieldGraft/ShieldGraftAbility.h"
#include "gameplay/ability/emberSwarm/EmberSwarmAbility.h"
#include "gameplay/ability/lanceDrive/LanceDriveAbility.h"
#include "gameplay/ability/lanceDrive/LanceDriveActor.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolAbility.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerRepairKitActor.h"
#include "gameplay/ability/inertialWake/InertialWakeActor.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceAbility.h"
#include "gameplay/ability/temporalRecall/TemporalRecallAbility.h"
#include "gameplay/ability/timeSlip/TimeSlipAbility.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceFieldActor.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaAbility.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaActor.h"
#include "gameplay/ability/arcScythes/ArcScythesBeamActor.h"
#include "gameplay/ability/strikeRun/StrikeRunBombardmentActor.h"
#include "gameplay/ability/chainLightning/ChainLightningAbility.h"
#include "gameplay/ability/stormMark/StormMarkAbility.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierAbility.h"
#include "gameplay/ability/glacialPressure/GlacialPressureAbility.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromAbility.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromFieldActor.h"
#include "gameplay/ability/cryoBola/CryoBolaAbility.h"
#include "gameplay/ability/cryoBola/CryoBolaProjectileActor.h"
#include "gameplay/ability/frozenThrong/FrozenThrongAbility.h"
#include "gameplay/ability/frozenThrong/FrozenThrongHuskActor.h"
#include "gameplay/ability/combatSentry/CombatSentryAbility.h"
#include "gameplay/ability/combatSentry/CombatSentryActor.h"
#include "gameplay/ability/combatSentry/CombatSentryProjectileActor.h"
#include "gameplay/ability/nanoPlague/NanoPlagueAbility.h"
#include "gameplay/ability/astralSurge/AstralSurgeAbility.h"
#include "gameplay/ability/astralSurge/AstralSurgeProjectileActor.h"
#include "gameplay/ability/wingSentinels/WingSentinelsAbility.h"
#include "gameplay/ability/wingSentinels/WingSentinelProjectileActor.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolAbility.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeAbility.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeActor.h"
#include "gameplay/ability/cryostasis/CryostasisAbility.h"
#include "gameplay/ability/voidGate/VoidGateAbility.h"
#include "gameplay/ability/voidGate/VoidGatePortalActor.h"
#include "gameplay/ability/shield/ShieldAbility.h"
#include "gameplay/ability/sunBeam/SunBeamAbility.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/effects/content/barrier/BarrierEffectBehavior.h"
#include "gameplay/effects/content/cryostasis/CryostasisIceShellEffectBehavior.h"
#include "gameplay/effects/content/directionalBarrier/DirectionalBarrierEffectBehavior.h"
#include "gameplay/effects/content/damageReduction/DamageReductionEffectBehavior.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "presentation/ability/AbilityPresentationContent.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualContent.h"

#include <functional>
#include <initializer_list>

namespace ly
{
	namespace
	{
		struct RegistrationStep
		{
			const char* name;
			std::function<bool()> registerStep;
		};

		bool RegisterAll(const std::initializer_list<RegistrationStep>& steps)
		{
			bool allRegistered = true;
			for (const RegistrationStep& step : steps)
			{
				if (step.registerStep())
				{
					continue;
				}
				LY_GAME_ERROR("Failed to register gameplay content step: %s", step.name);
				allRegistered = false;
			}
			return allRegistered;
		}

		template <typename Behavior>
		bool RegisterBehavior(const AbilityBehaviorType type)
		{
			return GameAbilityBehaviorRegistry::Register(
				type,
				[] { return std::make_unique<Behavior>(); }
			);
		}

		bool RegisterGameAbilityBehaviors()
		{
			return RegisterAll({
				{ "Configured", [] { return RegisterBehavior<GameAbilityBehavior>(AbilityBehaviorType::Configured); } },
				{ "Dash", [] { return RegisterBehavior<DashAbility>(AbilityBehaviorType::Dash); } },
				{ "Shield", [] { return RegisterBehavior<ShieldAbility>(AbilityBehaviorType::Shield); } },
				{ "GravityAnomaly", [] { return RegisterBehavior<GravityAnomalyAbility>(AbilityBehaviorType::GravityAnomaly); } },
				{ "Rocket", [] { return RegisterBehavior<RocketAbility>(AbilityBehaviorType::Rocket); } },
				{ "SunBeam", [] { return RegisterBehavior<SunBeamAbility>(AbilityBehaviorType::SunBeam); } },
				{ "InfernoSpray", [] { return RegisterBehavior<InfernoSprayAbility>(AbilityBehaviorType::InfernoSpray); } },
				{ "OverdriveCore", [] { return RegisterBehavior<OverdriveCoreAbility>(AbilityBehaviorType::OverdriveCore); } },
				{ "NullPulse", [] { return RegisterBehavior<NullPulseAbility>(AbilityBehaviorType::NullPulse); } },
				{ "PhaseDrift", [] { return RegisterBehavior<PhaseDriftAbility>(AbilityBehaviorType::PhaseDrift); } },
				{ "ShieldHarvest", [] { return RegisterBehavior<ShieldHarvestAbility>(AbilityBehaviorType::ShieldHarvest); } },
				{ "HullShock", [] { return RegisterBehavior<HullShockAbility>(AbilityBehaviorType::HullShock); } },
				{ "OrbitalDrones", [] { return RegisterBehavior<OrbitalDronesAbility>(AbilityBehaviorType::OrbitalDrones); } },
				{ "ExecutionDrive", [] { return RegisterBehavior<ExecutionDriveAbility>(AbilityBehaviorType::ExecutionDrive); } },
				{ "RelayPrism", [] { return RegisterBehavior<RelayPrismAbility>(AbilityBehaviorType::RelayPrism); } },
				{ "EchoProtocol", [] { return RegisterBehavior<EchoProtocolAbility>(AbilityBehaviorType::EchoProtocol); } },
				{ "RailBurst", [] { return RegisterBehavior<RailBurstAbility>(AbilityBehaviorType::RailBurst); } },
				{ "CrescentReaver", [] { return RegisterBehavior<CrescentReaverAbility>(AbilityBehaviorType::CrescentReaver); } },
				{ "AegisReaver", [] { return RegisterBehavior<AegisReaverAbility>(AbilityBehaviorType::AegisReaver); } },
				{ "MineLayer", [] { return RegisterBehavior<MineLayerAbility>(AbilityBehaviorType::MineLayer); } },
				{ "EnergySpear", [] { return RegisterBehavior<EnergySpearAbility>(AbilityBehaviorType::EnergySpear); } },
				{ "ScorchDrive", [] { return RegisterBehavior<ScorchDriveAbility>(AbilityBehaviorType::ScorchDrive); } },
				{ "IonStorm", [] { return RegisterBehavior<IonStormAbility>(AbilityBehaviorType::IonStorm); } },
				{ "ChainLightning", [] { return RegisterBehavior<ChainLightningAbility>(AbilityBehaviorType::ChainLightning); } },
				{ "StormMark", [] { return RegisterBehavior<StormMarkAbility>(AbilityBehaviorType::StormMark); } },
				{ "DirectionalBarrier", [] { return RegisterBehavior<DirectionalBarrierAbility>(AbilityBehaviorType::DirectionalBarrier); } },
				{ "GlacialPressure", [] { return RegisterBehavior<GlacialPressureAbility>(AbilityBehaviorType::GlacialPressure); } },
				{ "Cryostasis", [] { return RegisterBehavior<CryostasisAbility>(AbilityBehaviorType::Cryostasis); } },
				{ "VoidGate", [] { return RegisterBehavior<VoidGateAbility>(AbilityBehaviorType::VoidGate); } },
				{ "FrostMaelstrom", [] { return RegisterBehavior<FrostMaelstromAbility>(AbilityBehaviorType::FrostMaelstrom); } },
				{ "FrozenThrong", [] { return RegisterBehavior<FrozenThrongAbility>(AbilityBehaviorType::FrozenThrong); } },
				{ "CombatSentry", [] { return RegisterBehavior<CombatSentryAbility>(AbilityBehaviorType::CombatSentry); } },
				{ "AstralSurge", [] { return RegisterBehavior<AstralSurgeAbility>(AbilityBehaviorType::AstralSurge); } },
				{ "WingSentinels", [] { return RegisterBehavior<WingSentinelsAbility>(AbilityBehaviorType::WingSentinels); } },
				{ "NanoPlague", [] { return RegisterBehavior<NanoPlagueAbility>(AbilityBehaviorType::NanoPlague); } },
				{ "ReturnProtocol", [] { return RegisterBehavior<ReturnProtocolAbility>(AbilityBehaviorType::ReturnProtocol); } },
				{ "CrystalBarricade", [] { return RegisterBehavior<CrystalBarricadeAbility>(AbilityBehaviorType::CrystalBarricade); } }
				,
				{ "SolarBombardment", [] { return RegisterBehavior<SolarBombardmentAbility>(AbilityBehaviorType::SolarBombardment); } },
				{ "StrikeRun", [] { return RegisterBehavior<StrikeRunAbility>(AbilityBehaviorType::StrikeRun); } },
				{ "Blastback", [] { return RegisterBehavior<BlastbackAbility>(AbilityBehaviorType::Blastback); } }
				,
				{ "SeismicCharge", [] { return RegisterBehavior<SeismicChargeAbility>(AbilityBehaviorType::SeismicCharge); } },
				{ "ArcScythes", [] { return RegisterBehavior<ArcScythesAbility>(AbilityBehaviorType::ArcScythes); } }
				,
				{ "IroncladProtocol", [] { return RegisterBehavior<IroncladProtocolAbility>(AbilityBehaviorType::IroncladProtocol); } },
				{ "ClosedCircuit", [] { return RegisterBehavior<ClosedCircuitAbility>(AbilityBehaviorType::ClosedCircuit); } },
				{ "VectorSync", [] { return RegisterBehavior<VectorSyncAbility>(AbilityBehaviorType::VectorSync); } },
				{ "ZeroDrag", [] { return RegisterBehavior<ZeroDragAbility>(AbilityBehaviorType::ZeroDrag); } },
				{ "TemporalConvergence", [] { return RegisterBehavior<TemporalConvergenceAbility>(AbilityBehaviorType::TemporalConvergence); } }
				,
				{ "TemporalRecall", [] { return RegisterBehavior<TemporalRecallAbility>(AbilityBehaviorType::TemporalRecall); } }
				,
				{ "TimeSlip", [] { return RegisterBehavior<TimeSlipAbility>(AbilityBehaviorType::TimeSlip); } }
				,
				{ "FoldspaceArena", [] { return RegisterBehavior<FoldspaceArenaAbility>(AbilityBehaviorType::FoldspaceArena); } }
				,
				{ "CryoBola", [] { return RegisterBehavior<CryoBolaAbility>(AbilityBehaviorType::CryoBola); } }
				,
				{ "InertialWake", [] { return RegisterBehavior<InertialWakeAbility>(AbilityBehaviorType::InertialWake); } }
				,
				{ "ShieldGraft", [] { return RegisterBehavior<ShieldGraftAbility>(AbilityBehaviorType::ShieldGraft); } },
				{ "EmberSwarm", [] { return RegisterBehavior<EmberSwarmAbility>(AbilityBehaviorType::EmberSwarm); } },
				{ "LanceDrive", [] { return RegisterBehavior<LanceDriveAbility>(AbilityBehaviorType::LanceDrive); } },
				{ "ReclaimerProtocol", [] { return RegisterBehavior<ReclaimerProtocolAbility>(AbilityBehaviorType::ReclaimerProtocol); } }
			});
		}

		bool RegisterGameAbilityActors()
		{
			return RegisterAll({
				{ "GravityAnomalyProjectileActor", RegisterGravityAnomalyProjectileActorType },
				{ "GravityAnomalyFieldActor", RegisterGravityAnomalyFieldActorType },
				{ "RocketProjectileActor", RegisterRocketProjectileActorType },
				{ "OverdriveCoreProjectileActor", RegisterOverdriveCoreProjectileActorType },
				{ "SunBeamStrikeActor", RegisterSunBeamStrikeActorType },
				{ "InfernoSprayActor", RegisterInfernoSprayActorType },
				{ "RelayPrismActor", RegisterRelayPrismActorType },
				{ "RailBurstProjectileActor", RegisterRailBurstProjectileActorType },
				{ "CrescentReaverProjectileActor", RegisterCrescentReaverProjectileActorType },
				{ "AegisReaverProjectileActor", RegisterAegisReaverProjectileActorType },
				{ "MineLayerMineActor", RegisterMineLayerMineActorType },
				{ "ScorchDriveFireSegmentActor", RegisterScorchDriveFireSegmentActorType },
				{ "IonStormProjectileActor", RegisterIonStormProjectileActorType },
				{ "IonStormFieldActor", RegisterIonStormFieldActorType },
				{ "VoidGatePortalActor", RegisterVoidGatePortalActorType },
				{ "FrostMaelstromFieldActor", RegisterFrostMaelstromFieldActorType },
				{ "FrozenThrongHuskActor", RegisterFrozenThrongHuskActorType },
				{ "CombatSentryTurretActor", RegisterCombatSentryTurretActorType },
				{ "CombatSentryProjectileActor", RegisterCombatSentryProjectileActorType },
				{ "AstralSurgeProjectileActor", RegisterAstralSurgeProjectileActorType },
				{ "WingSentinelProjectileActor", RegisterWingSentinelProjectileActorType },
				{ "CrystalBarricadeWallActor", RegisterCrystalBarricadeWallActorType }
				,
				{ "SolarBombardmentProjectileActor", RegisterSolarBombardmentProjectileActorType },
				{ "StrikeRunBombardmentActor", RegisterStrikeRunBombardmentActorType }
				,
				{ "SeismicChargeBombActor", RegisterSeismicChargeBombActorType },
				{ "ArcScythesBeamActor", RegisterArcScythesBeamActorType },
				{ "TemporalConvergenceFieldActor", RegisterTemporalConvergenceFieldActorType },
				{ "ClosedCircuitFieldActor", RegisterClosedCircuitFieldActorType }
				,
				{ "FoldspaceArenaActor", RegisterFoldspaceArenaActorType }
				,
				{ "CryoBolaProjectileActor", RegisterCryoBolaProjectileActorType }
				,
				{ "InertialWakeActor", RegisterInertialWakeActorType },
				{ "LanceDriveActor", RegisterLanceDriveActorType },
				{ "ReclaimerRepairKitActor", RegisterReclaimerRepairKitActorType }
			});
		}

		bool RegisterGameAbilityEffects()
		{
			return RegisterAll({
				{ "GravityAnomalyEffectVisuals", RegisterGravityAnomalyEffectVisuals },
				{ "ShieldVisuals", RegisterShieldVisuals },
				{ "BarrierEffectBehavior", BarrierEffectBehavior::RegisterBarrierEffectBehavior },
				{ "DamageEffectBehaviors", DamageTypeSystem::RegisterDamageEffectBehaviors },
				{ "GravityAnomalyEffectBehavior", GravityAnomalyEffectBehavior::RegisterGravityAnomalyEffectBehavior },
				{ "DirectionalBarrierEffectBehavior", DirectionalBarrierEffectBehavior::RegisterDirectionalBarrierEffectBehavior },
				{ "CryostasisIceShellEffectBehavior", CryostasisIceShellEffectBehavior::RegisterCryostasisIceShellEffectBehavior }
				,
				{ "DamageReductionEffectBehavior", DamageReductionEffectBehavior::RegisterDamageReductionEffectBehavior }
			});
		}
	}

	bool RegisterGameAbilityContent()
	{
		static const bool registered =
			RegisterGameAbilityPresentationContent() &&
			RegisterGameAbilityBehaviors() &&
			RegisterGameAbilityActors() &&
			RegisterGameAbilityEffects();
		return registered;
	}
}
