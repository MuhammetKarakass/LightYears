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
#include "gameplay/content/EffectContentCatalog.h"
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

			return weaponsLoaded && shipsLoaded && abilitiesLoaded && effectsLoaded &&
				abilityContentRegistered && abilitiesValidated && effectsValidated;
		}();
		return registered;
	}
}
