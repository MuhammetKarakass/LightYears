#include "gameplay/content/GameContentBootstrap.h"

#include "effects/GameplayEffectDefinitionValidation.h"
#include "framework/AssetManager.h"
#include "framework/Core.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/content/GameAbilityContentRegistration.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

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

		void LogContentLoadFailure(
			const char* contentName,
			const std::string& failureReason
		)
		{
			LY_GAME_ERROR("Failed to load %s content: %s", contentName, failureReason.c_str());
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

			std::string damageStatusBalanceLoadFailureReason;
			const bool damageStatusBalanceLoaded =
				content::DamageStatusBalanceCatalog::LoadFromFile(
					assetRoot / "content/data/damage_status_balance.json",
					&damageStatusBalanceLoadFailureReason
				);
			if (!damageStatusBalanceLoaded)
			{
				LogContentLoadFailure(
					"damage status balance",
					damageStatusBalanceLoadFailureReason
				);
			}

			std::string weaponLoadFailureReason;
			const bool weaponsLoaded = content::WeaponContentCatalog::LoadFromFile(
				assetRoot / "content/data/weapons.json",
				&weaponLoadFailureReason
			);
			if (!weaponsLoaded)
			{
				LogContentLoadFailure("weapon", weaponLoadFailureReason);
			}

			std::string shipLoadFailureReason;
			const bool shipsLoaded = content::ShipContentCatalog::LoadFromFile(
				assetRoot / "content/data/ships.json",
				&shipLoadFailureReason
			);
			if (!shipsLoaded)
			{
				LogContentLoadFailure("ship", shipLoadFailureReason);
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
				LogContentLoadFailure("ability", abilityLoadFailureReason);
			}

			std::string effectLoadFailureReason;
			const bool effectsLoaded = content::EffectContentCatalog::LoadFromFile(
				assetRoot / "content/data/effects.json",
				EffectData::GetBuiltinGameplayEffectDefinitions(),
				&effectLoadFailureReason
			);
			if (!effectsLoaded)
			{
				LogContentLoadFailure("gameplay effect", effectLoadFailureReason);
			}

			// Concrete feature registration happens before shipped definitions are
			// validated. The bootstrap only owns startup orchestration.
			const bool abilityContentRegistered = RegisterGameAbilityContent();

			std::string abilityValidationFailureReason;
			const bool abilitiesValidated = abilityContentRegistered &&
				ValidateShippedAbilities(&abilityValidationFailureReason);
			if (!abilitiesValidated && !abilityValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped ability validation failed: %s",
					abilityValidationFailureReason.c_str()
				);
			}

			std::string effectValidationFailureReason;
			const bool effectsValidated = abilityContentRegistered &&
				ValidateShippedEffects(&effectValidationFailureReason);
			if (!effectsValidated && !effectValidationFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Shipped gameplay effect validation failed: %s",
					effectValidationFailureReason.c_str()
				);
			}

			std::string enemyProfileLoadFailureReason;
			const bool enemyProfilesLoaded = content::EnemyCombatProfileCatalog::LoadFromFile(
				assetRoot / "content/data/enemy_combat_profiles.json",
				&enemyProfileLoadFailureReason
			);
			if (!enemyProfilesLoaded)
			{
				LogContentLoadFailure("enemy combat profile", enemyProfileLoadFailureReason);
			}

			std::string enemyContentLoadFailureReason;
			const bool enemyContentLoaded = enemyProfilesLoaded &&
				content::EnemyContentCatalog::LoadFromFiles(
					assetRoot / "content/data/enemy_definitions.json",
					assetRoot / "content/data/enemy_behavior_profiles.json",
					&enemyContentLoadFailureReason
				);
			if (!enemyContentLoaded && !enemyContentLoadFailureReason.empty())
			{
				LY_GAME_ERROR(
					"Enemy content validation failed: %s",
					enemyContentLoadFailureReason.c_str()
				);
			}

			return damageStatusBalanceLoaded && weaponsLoaded && shipsLoaded &&
				abilitiesLoaded && effectsLoaded &&
				abilityContentRegistered && abilitiesValidated && effectsValidated &&
				enemyProfilesLoaded && enemyContentLoaded;
		}();
		return registered;
	}
}
