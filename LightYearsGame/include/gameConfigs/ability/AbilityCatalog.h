#pragma once

#include "gameConfigs/ability/movement/DashConfig.h"
#include "gameConfigs/ability/utility/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/OverdriveCoreConfig.h"
#include "gameConfigs/ability/offensive/RailBurstConfig.h"
#include "gameConfigs/ability/offensive/CrescentReaverConfig.h"
#include "gameConfigs/ability/offensive/AegisReaverConfig.h"
#include "gameConfigs/ability/offensive/MineLayerConfig.h"
#include "gameConfigs/ability/movement/ScorchDriveConfig.h"
#include "gameConfigs/ability/offensive/IonStormConfig.h"
#include "gameConfigs/ability/offensive/InertialWakeConfig.h"
#include "gameConfigs/ability/movement/LanceDriveConfig.h"
#include "gameConfigs/ability/offensive/SolarBombardmentConfig.h"
#include "gameConfigs/ability/offensive/StrikeRunConfig.h"
#include "gameConfigs/ability/offensive/BlastbackConfig.h"
#include "gameConfigs/ability/offensive/SeismicChargeConfig.h"
#include "gameConfigs/ability/offensive/ArcScythesConfig.h"
#include "gameConfigs/ability/offensive/ChainLightningConfig.h"
#include "gameConfigs/ability/offensive/StormMarkConfig.h"
#include "gameConfigs/ability/movement/EnergySpearConfig.h"
#include "gameConfigs/ability/offensive/ExecutionDriveConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameConfigs/ability/defensive/DirectionalBarrierConfig.h"
#include "gameConfigs/ability/defensive/IroncladProtocolConfig.h"
#include "gameConfigs/ability/defensive/ClosedCircuitConfig.h"
#include "gameConfigs/ability/defensive/ShieldGraftConfig.h"
#include "gameConfigs/ability/defensive/ReclaimerProtocolConfig.h"
#include "gameConfigs/ability/defensive/TemporalRecallConfig.h"
#include "gameConfigs/ability/defensive/TimeSlipConfig.h"
#include "gameConfigs/ability/movement/PhaseDriftConfig.h"
#include "gameConfigs/ability/movement/VectorSyncConfig.h"
#include "gameConfigs/ability/movement/ZeroDragConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/ability/offensive/GlacialPressureConfig.h"
#include "gameConfigs/ability/offensive/FrozenThrongConfig.h"
#include "gameConfigs/ability/offensive/CombatSentryConfig.h"
#include "gameConfigs/ability/offensive/NanoPlagueConfig.h"
#include "gameConfigs/ability/utility/EchoProtocolConfig.h"
#include "gameConfigs/ability/utility/VoidGateConfig.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData
{
	namespace Definitions
	{
		inline vega::GameAbilityDefinition MakePrimaryFireAbilityDefinition(
			const PrimaryWeaponDefinition& weaponDefinition)
		{
			const bool automaticFire = weaponDefinition.automaticFire;
			const auto toAbilitySteps = [](const vega::List<PrimaryWeaponLevelStep>& weaponSteps)
			{
				vega::List<vega::AbilityLevelStep> abilitySteps;
				for (const PrimaryWeaponLevelStep& weaponLevelStep : weaponSteps)
				{
					vega::AbilityLevelStep abilityLevelStep;
					abilityLevelStep.attributeModifiers = weaponLevelStep.attributeModifiers;
					abilityLevelStep.scalingRules = weaponLevelStep.scalingRules;
					abilityLevelStep.unlockedUpgradeIds = weaponLevelStep.unlockedUpgradeIds;
					for (const PrimaryWeaponFeatureType featureType : weaponLevelStep.unlockedFeatureTypes)
					{
						abilityLevelStep.unlockedUpgradeIds.emplace_back(
							PrimaryWeaponFeatureUpgradeId(featureType)
						);
					}
					abilitySteps.push_back(abilityLevelStep);
				}
				return abilitySteps;
			};
			const ResolvedWeaponProgression resolvedProgression =
				weaponDefinition.progressionProfile.ResolveProgression();

			vega::GameAbilityDefinition definition;
			definition.abilityId = weaponDefinition.weaponId;
			definition.slot = sas::AbilitySlot::PrimaryFire;
			definition.activationPolicy = automaticFire
				? sas::AbilityActivationPolicy::WhileHeld
				: sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = automaticFire
				? sas::AbilityLifetimePolicy::WhileInputHeld
				: sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				vega::GameplayTagSchema::AbilityPrimary,
				vega::GameplayTagSchema::AbilityOffense
			};
			definition.displayName = weaponDefinition.weaponId;
			definition.iconPath = weaponDefinition.presentationDefinition.texturePath;
			definition.accentColor = sf::Color{ 100, 220, 255, 255 };
			definition.actions = {
				vega::AbilityActionSpec{
					automaticFire
						? sas::AbilityActionPhase::WhileActive
						: sas::AbilityActionPhase::OnActivate,
					vega::FireWeaponAction{ weaponDefinition },
					0.f,
					automaticFire ? 0 : 1
				}
			};
			definition.levelProgression = toAbilitySteps(resolvedProgression.prefix);
			definition.repeatingLevelProgression = toAbilitySteps(resolvedProgression.cycle);
			definition.scrapCost = weaponDefinition.progressionProfile.scrapCost;
			definition.damageTags = weaponDefinition.damageTags.empty()
				? vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Photonic }
				: weaponDefinition.damageTags;
			return definition;
		}
	}

	inline vega::GameAbilityDefinition MakePrimaryFireAbilityDefinition(
		const PrimaryWeaponDefinition& weaponDefinition)
	{
		return Definitions::MakePrimaryFireAbilityDefinition(weaponDefinition);
	}

	const vega::List<const vega::GameAbilityDefinition*>& GetBuiltinShippedAbilityDefinitions();
	const vega::List<const vega::AbilityActorDefinition*>& GetBuiltinAbilityActorDefinitions();
	const vega::List<const vega::GameAbilityDefinition*>& GetShippedAbilityDefinitions();
	const vega::GameAbilityDefinition* FindShippedAbilityDefinition(
		const std::string& abilityId
	);
}
