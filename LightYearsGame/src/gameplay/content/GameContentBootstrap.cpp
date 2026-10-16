#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/GameContentSchemas.h"

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
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/module/ShipSchematicValidator.h"

#include <filesystem>
#include <fstream>
#include <framework/EngineContext.h>
#include <framework/content/ContentSchema.h>

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

		bool ValidateShipSchematics(std::string* failureReason)
		{
			for (const std::string& shipId : content::ShipContentCatalog::GetPlayerShipIds())
			{
				const ShipDefinition* ship = content::ShipContentCatalog::FindById(shipId);
				if (!ship || !ship->schematic)
				{
					continue;
				}
				std::string schematicFailure;
				if (!ShipSchematicValidator::Validate(
					*ship->schematic,
					content::ModuleContentCatalog::GetDefinitions(),
					content::ModuleContentCatalog::GetZoneTable(),
					&schematicFailure
				))
				{
					if (failureReason)
					{
						*failureReason = "Ship " + shipId + " schematic: " + schematicFailure;
					}
					return false;
				}
			}
			return true;
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
		content::RegisterGameContentSchemas();
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

			std::string moduleLoadFailureReason;
			const bool modulesLoaded = content::ModuleContentCatalog::LoadFromFiles(
				assetRoot / "content/data/modules.json",
				assetRoot / "content/data/module_side_features.json",
				&moduleLoadFailureReason
			);
			if (!modulesLoaded)
			{
				LogContentLoadFailure("module", moduleLoadFailureReason);
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

			// Schematics reference module IDs, so they validate after both catalogs load.
			std::string schematicFailureReason;
			const bool schematicsValidated = modulesLoaded && shipsLoaded &&
				ValidateShipSchematics(&schematicFailureReason);
			if (modulesLoaded && shipsLoaded && !schematicsValidated)
			{
				LY_GAME_ERROR("Ship schematic validation failed: %s", schematicFailureReason.c_str());
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

			return damageStatusBalanceLoaded && weaponsLoaded && modulesLoaded && shipsLoaded &&
				schematicsValidated &&
				abilitiesLoaded && effectsLoaded &&
				abilityContentRegistered && abilitiesValidated && effectsValidated &&
				enemyProfilesLoaded && enemyContentLoaded;
		}();
		return registered;
	}
	void GameContentBootstrap::Reload(EngineContext& context, const std::vector<std::filesystem::path>& paths)
	{
		using namespace content;
		const auto runtimeRoot = std::filesystem::path{ context.Assets().GetAssetRootDirectory() };
		std::map<std::string, std::filesystem::path> requested;
		for (const auto& path : paths)
		{
			if (path.parent_path().filename() != "data" || path.parent_path().parent_path().filename() != "content") continue;
			requested[path.filename().u8string()] = path;
		}
		if (requested.empty()) return;
		const auto root = requested.begin()->second.parent_path().parent_path().parent_path();
		const auto resolve = [&](const std::string& relative)
		{
			const auto candidate = root / relative;
			return std::filesystem::exists(candidate) ? candidate : runtimeRoot / relative;
		};
		try
		{
			ContentDocuments documents;
			for (const auto& [type, schema] : context.ContentSchemas().Types())
			{
				std::ifstream input{ resolve(schema.file) };
				documents[type] = AuthoringJson::parse(input);
			}
			// Validate against the candidate documents before replacing any catalog.
			// Cross-catalog ability/effect and ship/module references see the same disk state.
			for (const auto& [type, schema] : context.ContentSchemas().Types())
			{
				if (schema.validate)
				{
					const auto error = schema.validate(documents, "");
					if (!error.empty()) { LY_GAME_ERROR("Content reload rejected (%s): %s", type.c_str(), error.c_str()); return; }
				}
			}
			if (requested.count("modules.json") || requested.count("module_side_features.json"))
			{
				std::string error;
				if (!ModuleContentCatalog::LoadFromFiles(resolve("content/data/modules.json"), resolve("content/data/module_side_features.json"), &error, true))
				{ LY_GAME_ERROR("Content reload rejected (modules): %s", error.c_str()); return; }
			}
			const auto reload = [&](const char* file, const auto& load)
			{
				if (!requested.count(file)) return;
				std::string error;
				if (load(error)) LY_GAME_INFO("Reloaded content: %s (future spawns/grants)", requested.at(file).generic_u8string().c_str());
				else LY_GAME_ERROR("Content reload rejected (%s): %s", file, error.c_str());
			};
			reload("weapons.json", [&](auto& error) { return WeaponContentCatalog::LoadFromFile(resolve("content/data/weapons.json"), &error, true); });
			if (requested.count("module_side_features.json")) requested["modules.json"] = resolve("content/data/modules.json");
			reload("modules.json", [&](auto& error) { return ModuleContentCatalog::LoadFromFiles(resolve("content/data/modules.json"), resolve("content/data/module_side_features.json"), &error); });
			reload("ships.json", [&](auto& error) { return ShipContentCatalog::LoadFromFile(resolve("content/data/ships.json"), &error, true); });
			reload("effects.json", [&](auto& error) { return EffectContentCatalog::LoadFromFile(resolve("content/data/effects.json"), EffectData::GetBuiltinGameplayEffectDefinitions(), &error, true); });
			reload("abilities.json", [&](auto& error) { return AbilityContentCatalog::LoadFromFile(resolve("content/data/abilities.json"), AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions(), &error, true); });
		}
		catch (const std::exception& error) { LY_GAME_ERROR("Content reload rejected: %s", error.what()); }
	}
}
