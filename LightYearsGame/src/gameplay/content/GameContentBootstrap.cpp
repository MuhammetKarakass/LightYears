#include "gameplay/content/GameContentBootstrap.h"

#include "framework/AssetManager.h"
#include "framework/Core.h"
#include "effects/GameplayEffectDefinitionValidation.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/ability/dash/DashAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/infernoSpray/InfernoSprayAbility.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreAbility.h"
#include "gameplay/ability/nullPulse/NullPulseAbility.h"
#include "gameplay/ability/phaseDrift/PhaseDriftAbility.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreProjectileActor.h"
#include "gameplay/ability/rocket/RocketAbility.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/shield/ShieldAbility.h"
#include "gameplay/ability/sunBeam/SunBeamAbility.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/effects/content/barrier/BarrierEffectBehavior.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "presentation/ability/AbilityPresentationContent.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualContent.h"

#include <filesystem>

namespace ly
{
	namespace
	{
		bool IsEffectBehaviorRegistered(const sas::GameplayEffectBehaviorKey& behaviorKey)
		{
			return GetEffectBehaviorRuntime().IsRegistered(behaviorKey);
		}

		bool ValidateEffectForRuntime(
			const sas::GameplayEffectDefinition& definition,
			std::string* failureReason
		)
		{
			return GameplayEffectDefinitionValidator::Validate(
				definition,
				IsEffectBehaviorRegistered,
				failureReason
			);
		}

		bool ValidateShippedAbilities(std::string* failureReason)
		{
			return GameAbilityDefinitionValidator::ValidateCatalog(
				AbilityData::GetShippedAbilityDefinitions(),
				ValidateEffectForRuntime,
				failureReason
			);
		}

		bool ValidateShippedEffects(std::string* failureReason)
		{
			return GameplayEffectDefinitionValidator::ValidateCatalog(
				EffectData::GetShippedGameplayEffectDefinitions(),
				IsEffectBehaviorRegistered,
				failureReason
			);
		}
	}

	bool GameContentBootstrap::Register()
	{
		static const bool registered = []
		{
			std::filesystem::path assetRoot =
				AssetManager::GetAssetManager().GetAssetRootDirectory();
			if (assetRoot.empty())
			{
				assetRoot = "LightYearsGame/assets";
			}

			std::string weaponLoadFailureReason;
			const bool weaponsLoaded = content::WeaponContentCatalog::LoadFromFile(
				assetRoot / "content/data/weapons.json",
				&weaponLoadFailureReason
			);
			if (!weaponsLoaded)
			{
				LY_GAME_ERROR("Failed to load weapon content: %s", weaponLoadFailureReason.c_str());
			}

			std::string shipLoadFailureReason;
			const bool shipsLoaded = content::ShipContentCatalog::LoadFromFile(
				assetRoot / "content/data/ships.json",
				&shipLoadFailureReason
			);
			if (!shipsLoaded)
			{
				LY_GAME_ERROR("Failed to load ship content: %s", shipLoadFailureReason.c_str());
			}

			std::string abilityLoadFailureReason;
			const bool abilitiesLoaded = content::AbilityContentCatalog::LoadFromFile(
				assetRoot / "content/data/abilities.json",
				AbilityData::GetBuiltinShippedAbilityDefinitions(),
				AbilityData::GetBuiltinAbilityActorDefinitions(),
				&abilityLoadFailureReason
			);
			if (!abilitiesLoaded)
			{
				LY_GAME_ERROR("Failed to load ability content: %s", abilityLoadFailureReason.c_str());
			}

			std::string effectLoadFailureReason;
			const bool effectsLoaded = content::EffectContentCatalog::LoadFromFile(
				assetRoot / "content/data/effects.json",
				EffectData::GetBuiltinGameplayEffectDefinitions(),
				&effectLoadFailureReason
			);
			if (!effectsLoaded)
			{
				LY_GAME_ERROR(
					"Failed to load gameplay effect content: %s",
					effectLoadFailureReason.c_str()
				);
			}

			const bool presentationRegistered = RegisterGameAbilityPresentationContent();
			const bool configuredRegistered = GameAbilityBehaviorRegistry::Register(
				AbilityBehaviorType::Configured,
				[] { return std::make_unique<GameAbilityBehavior>(); }
			);
			const bool abilitiesRegistered = configuredRegistered &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::Dash,
					[] { return std::make_unique<DashAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::Shield,
					[] { return std::make_unique<ShieldAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::GravityAnomaly,
					[] { return std::make_unique<GravityAnomalyAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::Rocket,
					[] { return std::make_unique<RocketAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::SunBeam,
					[] { return std::make_unique<SunBeamAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::InfernoSpray,
					[] { return std::make_unique<InfernoSprayAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::OverdriveCore,
					[] { return std::make_unique<OverdriveCoreAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::NullPulse,
					[] { return std::make_unique<NullPulseAbility>(); }
				) &&
				GameAbilityBehaviorRegistry::Register(
					AbilityBehaviorType::PhaseDrift,
					[] { return std::make_unique<PhaseDriftAbility>(); }
				);

			const bool abilityActorsRegistered =
				RegisterGravityAnomalyProjectileActorType() &&
				RegisterGravityAnomalyFieldActorType() &&
				RegisterRocketProjectileActorType() &&
				RegisterOverdriveCoreProjectileActorType() &&
				RegisterSunBeamStrikeActorType() &&
				RegisterInfernoSprayActorType();
			const bool effectsRegistered =
				RegisterGravityAnomalyEffectVisuals() &&
				RegisterShieldVisuals() &&
				BarrierEffectBehavior::RegisterBarrierEffectBehavior() &&
				DamageTypeSystem::RegisterDamageEffectBehaviors() &&
				GravityAnomalyEffectBehavior::RegisterGravityAnomalyEffectBehavior();

			std::string abilityValidationFailureReason;
			const bool abilitiesValidated = abilitiesRegistered &&
				ValidateShippedAbilities(&abilityValidationFailureReason);
			if (!abilitiesValidated && !abilityValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped ability validation failed: %s",
					abilityValidationFailureReason.c_str()
				);
			}

			std::string effectValidationFailureReason;
			const bool effectsValidated = ValidateShippedEffects(&effectValidationFailureReason);
			if (!effectsValidated && !effectValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped gameplay effect validation failed: %s",
					effectValidationFailureReason.c_str()
				);
			}

			return weaponsLoaded && shipsLoaded && abilitiesLoaded && effectsLoaded &&
				presentationRegistered && abilitiesValidated && abilityActorsRegistered &&
				effectsRegistered && effectsValidated;
		}();
		return registered;
	}
}
