#include "gameplay/content/AttachmentLoader.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/EffectLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/content/WeaponLoader.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/AbilityLoader.h"
#include "gameplay/content/EnemyCombatProfileLoader.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/weapon/internal/PrimaryWeaponDefinitionValidator.h"

#include "attributes/GameplayAttribute.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/movement/DashConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameConfigs/ability/defensive/DirectionalBarrierConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameConfigs/ability/control/FrostMaelstromConfig.h"
#include "gameConfigs/ability/offensive/FrozenThrongConfig.h"
#include "gameConfigs/ability/offensive/CombatSentryConfig.h"
#include "gameConfigs/ability/offensive/NanoPlagueConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/OverdriveCoreConfig.h"
#include "gameConfigs/ability/offensive/OrbitalDronesConfig.h"
#include "gameConfigs/ability/offensive/ExecutionDriveConfig.h"
#include "gameConfigs/ability/control/NullPulseConfig.h"
#include "gameConfigs/ability/movement/PhaseDriftConfig.h"
#include "gameConfigs/ability/offensive/HullShockConfig.h"
#include "gameConfigs/ability/defensive/ShieldHarvestConfig.h"
#include "gameConfigs/ability/defensive/CryostasisConfig.h"
#include "gameConfigs/ability/utility/RelayPrismConfig.h"
#include "gameConfigs/ability/utility/EchoProtocolConfig.h"
#include "gameConfigs/ability/utility/VoidGateConfig.h"
#include "gameConfigs/ability/offensive/RailBurstConfig.h"
#include "gameConfigs/ability/offensive/AstralSurgeConfig.h"
#include "gameConfigs/ability/offensive/WingSentinelsConfig.h"
#include "gameConfigs/ability/defensive/ReturnProtocolConfig.h"
#include "gameConfigs/ability/defensive/CrystalBarricadeConfig.h"
#include "gameConfigs/ability/offensive/CrescentReaverConfig.h"
#include "gameConfigs/ability/offensive/ScorchDriveConfig.h"
#include "gameConfigs/ability/offensive/IonStormConfig.h"
#include "gameConfigs/ability/offensive/ChainLightningConfig.h"
#include "gameConfigs/ability/offensive/GlacialPressureConfig.h"
#include "gameConfigs/ability/offensive/MineLayerConfig.h"
#include "gameConfigs/ability/movement/EnergySpearConfig.h"
#include "gameConfigs/combat/EffectConfig.h"

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <variant>

namespace ly
{
	weak_ptr<Reward> CreateRewardHealth(World*) { return {}; }
	weak_ptr<Reward> CreateRewardLife(World*) { return {}; }
	weak_ptr<Reward> CreateRewardShield(World*) { return {}; }
}

namespace
{
	bool NearlyEqual(float left, float right, float tolerance = 0.0001f)
	{
		return std::fabs(left - right) <= tolerance;
	}
}

namespace
{

	bool Fail(const char* message)
	{
		std::cerr << message << '\n';
		return false;
	}

	const PrimaryWeaponDefinition* FindWeapon(
		const ly::List<PrimaryWeaponDefinition>& definitions,
		const char* weaponId
	)
	{
		for (const PrimaryWeaponDefinition& definition : definitions)
		{
			if (definition.weaponId == weaponId)
			{
				return &definition;
			}
		}
		return nullptr;
	}
}

namespace
{
	int RunEnemyCombatProfileContentTests()
	{
		const std::filesystem::path weaponPath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/weapons.json";
		const std::filesystem::path enemyProfilePath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/enemy_combat_profiles.json";
		const std::filesystem::path abilitiesPath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/abilities.json";
		const std::filesystem::path shipsPath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/ships.json";

		if (!ly::content::WeaponContentCatalog::IsLoaded())
		{
			ly::content::WeaponContentCatalog::LoadFromFile(weaponPath);
		}
		if (!ly::content::AbilityContentCatalog::IsLoaded())
		{
			std::string abilityLoadErr;
			if (!ly::content::AbilityContentCatalog::LoadFromFile(
				abilitiesPath,
				AbilityData::GetBuiltinShippedAbilityDefinitions(),
				AbilityData::GetBuiltinAbilityActorDefinitions(),
				&abilityLoadErr))
			{
				return Fail(("Failed to load AbilityContentCatalog: " + abilityLoadErr).c_str()) ? 0 : 1;
			}
		}

		const ly::content::EnemyCombatProfileLoader::Result loadedEnemyProfiles =
			ly::content::EnemyCombatProfileLoader::LoadFromFile(enemyProfilePath);
		if (!loadedEnemyProfiles.Succeeded())
		{
			return Fail(loadedEnemyProfiles.error.c_str()) ? 0 : 1;
		}
		if (loadedEnemyProfiles.profiles.size() != 3)
		{
			return Fail("Enemy combat profiles did not load exactly three profiles") ? 0 : 1;
		}

		std::string enemyCatalogError;
		if (!ly::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath, &enemyCatalogError))
		{
			return Fail(enemyCatalogError.c_str()) ? 0 : 1;
		}

	const ly::EnemyCombatProfile* vanguardProfile =
			ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
		const ly::EnemyCombatProfile* twinBladeProfile =
			ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.StrafeSkirmisher.Basic");
		const ly::EnemyCombatProfile* hexagonProfile =
			ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.RangeKeeper.Basic");

		if (!vanguardProfile || !twinBladeProfile || !hexagonProfile)
		{
			return Fail("Catalog failed to find one of the three basic enemy profiles") ? 0 : 1;
		}

		if (vanguardProfile->weapons.size() != 1 || vanguardProfile->weapons.front().weaponId != "Weapon.Projectile.EnemyVanguardPulse.Basic" ||
			vanguardProfile->powerScalingPolicy != ly::EnemyPowerScalingPolicy::Disabled)
		{
			return Fail("Vanguard enemy combat profile has unexpected weapon or scaling policy") ? 0 : 1;
		}

		// Duplicate enemy profile ID rejected
		const std::filesystem::path duplicateProfilePath =
			std::filesystem::temp_directory_path() / "lightyears_dup_enemy_profile.json";
		{
			std::ofstream dupFile{ duplicateProfilePath };
			dupFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    { "id": "EnemyCombat.Test.Basic", "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }] },
	    { "id": "EnemyCombat.Test.Basic", "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }] }
	  ]
	})";
		}
		const ly::content::EnemyCombatProfileLoader::Result dupLoadResult =
			ly::content::EnemyCombatProfileLoader::LoadFromFile(duplicateProfilePath);
		std::filesystem::remove(duplicateProfilePath);
		if (dupLoadResult.Succeeded() || dupLoadResult.error.find("Duplicate") == std::string::npos)
		{
			return Fail("Enemy combat profile loader accepted duplicate profile IDs") ? 0 : 1;
		}

		// Missing primary weapon rejected
		const std::filesystem::path missingWeaponPath =
			std::filesystem::temp_directory_path() / "lightyears_missing_weapon_profile.json";
		{
			std::ofstream mwFile{ missingWeaponPath };
			mwFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    { "id": "EnemyCombat.Test.Basic", "weapons": [{ "weaponId": "Weapon.Projectile.NonExistent.Basic", "slot": "PrimaryFire" }] }
	  ]
	})";
		}
		std::string missingWeaponError;
		const bool missingWeaponLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(missingWeaponPath, &missingWeaponError);
		std::filesystem::remove(missingWeaponPath);
		if (missingWeaponLoaded || missingWeaponError.find("not found") == std::string::npos)
		{
			return Fail("Enemy combat profile catalog accepted nonexistent primary weapon") ? 0 : 1;
		}

		// Missing ability rejected
		const std::filesystem::path missingAbilityPath =
			std::filesystem::temp_directory_path() / "lightyears_missing_ability_profile.json";
		{
			std::ofstream maFile{ missingAbilityPath };
			maFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }],
	      "abilities": [{ "abilityId": "Ability.Offense.NonExistent.Basic", "slot": "Ability1" }]
	    }
	  ]
	})";
		}
		std::string missingAbilityError;
		const bool missingAbilityLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(missingAbilityPath, &missingAbilityError);
		std::filesystem::remove(missingAbilityPath);
		if (missingAbilityLoaded || missingAbilityError.find("not found") == std::string::npos)
		{
			return Fail("Enemy combat profile catalog accepted nonexistent ability") ? 0 : 1;
		}

		// Duplicate ability slot rejected
		const std::filesystem::path dupSlotPath =
			std::filesystem::temp_directory_path() / "lightyears_dup_slot_profile.json";
		{
			std::ofstream dsFile{ dupSlotPath };
			dsFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }],
	      "abilities": [
	        { "abilityId": "Ability.Offense.Rocket.Basic", "slot": "Ability1" },
	        { "abilityId": "Ability.Offense.Rocket.Basic", "slot": "Ability1" }
	      ]
	    }
	  ]
	})";
		}
		std::string dupSlotError;
		const bool dupSlotLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(dupSlotPath, &dupSlotError);
		std::filesystem::remove(dupSlotPath);
		if (dupSlotLoaded || dupSlotError.find("slot") == std::string::npos)
		{
			return Fail("Enemy combat profile catalog accepted duplicate ability slots") ? 0 : 1;
		}

		// PowerScalingPolicy::Disabled rejects AP/EP scaling content
		const std::filesystem::path disabledScalingPath =
			std::filesystem::temp_directory_path() / "lightyears_disabled_scaling_profile.json";
		{
			std::ofstream disFile{ disabledScalingPath };
			disFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.FighterRapidLaser.Basic", "slot": "PrimaryFire" }],
	      "powerScalingPolicy": "Disabled"
	    }
	  ]
	})";
		}
		std::string disabledScalingError;
		const bool disabledScalingLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(disabledScalingPath, &disabledScalingError);
		std::filesystem::remove(disabledScalingPath);
		if (disabledScalingLoaded || disabledScalingError.find("scaling") == std::string::npos)
		{
			return Fail("Enemy combat profile catalog accepted AP/EP scaling under Disabled policy") ? 0 : 1;
		}

		// PowerScalingPolicy::Allowed accepts AP/EP scaling content without granting automatic AP/EP
		const std::filesystem::path allowedScalingPath =
			std::filesystem::temp_directory_path() / "lightyears_allowed_scaling_profile.json";
		{
			std::ofstream allowFile{ allowedScalingPath };
			allowFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.FighterRapidLaser.Basic", "slot": "PrimaryFire" }],
	      "powerScalingPolicy": "Allowed"
	    }
	  ]
	})";
		}
		std::string allowedScalingError;
		const bool allowedScalingLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(allowedScalingPath, &allowedScalingError);
		std::filesystem::remove(allowedScalingPath);
		if (!allowedScalingLoaded)
		{
			return Fail(allowedScalingError.c_str()) ? 0 : 1;
		}
		const ly::EnemyCombatProfile* allowedProfile =
			ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.Test.Basic");
		if (!allowedProfile || allowedProfile->powerScalingPolicy != ly::EnemyPowerScalingPolicy::Allowed)
		{
			return Fail("Allowed profile policy not preserved") ? 0 : 1;
		}

		// Duplicate ability ID in different slots is rejected
		const std::filesystem::path dupAbilityIdPath =
			std::filesystem::temp_directory_path() / "lightyears_dup_ability_id_profile.json";
		{
			std::ofstream dupFile{ dupAbilityIdPath };
			dupFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }],
	      "abilities": [
	        { "abilityId": "Ability.Movement.Dash.Basic", "slot": "Ability1" },
	        { "abilityId": "Ability.Movement.Dash.Basic", "slot": "Ability2" }
	      ]
	    }
	  ]
	})";
		}
		std::string dupAbilityIdError;
		const bool dupAbilityIdLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(dupAbilityIdPath, &dupAbilityIdError);
		std::filesystem::remove(dupAbilityIdPath);
		if (dupAbilityIdLoaded || dupAbilityIdError.find("Duplicate ability ID") == std::string::npos)
		{
			return Fail("Enemy combat profile catalog accepted duplicate ability ID across slots") ? 0 : 1;
		}

		// The removed singular primaryWeaponId must not silently fall back.
		const std::filesystem::path legacyKeyPath =
			std::filesystem::temp_directory_path() / "lightyears_legacy_key_profile.json";
		{
			std::ofstream legacyFile{ legacyKeyPath };
			legacyFile << R"({
		  "schemaVersion": 1,
		  "profiles": [
		    {
      "id": "EnemyCombat.Test.Basic",
		      "primaryWeaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic"
		    }
		  ]
		})";
		}
		ly::EnemyCombatProfile invalidEnemyGrowth = *vanguardProfile;
		invalidEnemyGrowth.profileId = "EnemyCombat.Test.InvalidGrowth";
		invalidEnemyGrowth.progression.naturalGrowth.push_back({ ly::OwnerAttributeIds::AttackPower, 1.f });
		std::string invalidGrowthError;
		if (ly::content::EnemyCombatProfileCatalog::ValidateProfile(invalidEnemyGrowth, &invalidGrowthError) || invalidGrowthError.find("Owner.MaxHealth") == std::string::npos)
		{
			return Fail("Enemy catalog accepted a non-whitelisted natural growth attribute") ? 0 : 1;
		}
		std::string legacyKeyError;
		const bool legacyKeyLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(legacyKeyPath, &legacyKeyError);
		std::filesystem::remove(legacyKeyPath);
		if (legacyKeyLoaded || legacyKeyError.find("Unknown key 'primaryWeaponId'") == std::string::npos)
		{
			return Fail("Enemy combat profile loader accepted removed primaryWeaponId") ? 0 : 1;
		}

		// Catalog validation is generic and must not require a hardcoded roster.
		std::string shippedValErr;
		if (!ly::content::EnemyCombatProfileCatalog::ValidateShippedProfiles(&shippedValErr))
		{
			return Fail(("Generic enemy combat profile validation failed: " + shippedValErr).c_str()) ? 0 : 1;
		}

		// Valid multi-ability profile without power scaling is accepted
		const std::filesystem::path multiAbilityPath =
			std::filesystem::temp_directory_path() / "lightyears_multi_ability_profile.json";
		{
			std::ofstream multiFile{ multiAbilityPath };
			multiFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "weapons": [{ "weaponId": "Weapon.Projectile.EnemyVanguardPulse.Basic", "slot": "PrimaryFire" }],
	      "abilities": [
	        { "abilityId": "Ability.Movement.Dash.Basic", "slot": "Ability1" },
	        { "abilityId": "Ability.Defense.Shield.Basic", "slot": "Ability2" }
	      ]
	    }
	  ]
	})";
		}
		std::string multiAbilityError;
		const bool multiAbilityLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(multiAbilityPath, &multiAbilityError);
		std::filesystem::remove(multiAbilityPath);
		if (!multiAbilityLoaded)
		{
			return Fail(("Valid multi-ability profile rejected: " + multiAbilityError).c_str()) ? 0 : 1;
		}

		// Profile with no attack method and allowContactDamageOnly=false is rejected
		const std::filesystem::path noAttackPath =
			std::filesystem::temp_directory_path() / "lightyears_no_attack_profile.json";
		{
			std::ofstream noAttFile{ noAttackPath };
			noAttFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic"
	    }
	  ]
	})";
		}
		std::string noAttackError;
		const bool noAttackLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(noAttackPath, &noAttackError);
		std::filesystem::remove(noAttackPath);
		if (noAttackLoaded || noAttackError.find("no attack method") == std::string::npos)
		{
			return Fail("Profile with no attack method was accepted when allowContactDamageOnly is false") ? 0 : 1;
		}

		// Profile with contact damage only is accepted when allowContactDamageOnly=true
		const std::filesystem::path contactOnlyPath =
			std::filesystem::temp_directory_path() / "lightyears_contact_only_profile.json";
		{
			std::ofstream conFile{ contactOnlyPath };
			conFile << R"({
	  "schemaVersion": 1,
	  "profiles": [
	    {
      "id": "EnemyCombat.Test.Basic",
	      "allowContactDamageOnly": true
	    }
	  ]
	})";
		}
		std::string contactOnlyError;
		const bool contactOnlyLoaded =
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(contactOnlyPath, &contactOnlyError);
		std::filesystem::remove(contactOnlyPath);
		if (!contactOnlyLoaded)
		{
			return Fail(("Contact damage only profile was rejected: " + contactOnlyError).c_str()) ? 0 : 1;
		}

		// Restore original catalog
		ly::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath);

		// Verify that the restored shipped catalog validates successfully
		if (!ly::content::EnemyCombatProfileCatalog::ValidateShippedProfiles(&shippedValErr))
		{
			return Fail(("Shipped catalog failed validation after restoration: " + shippedValErr).c_str()) ? 0 : 1;
		}

		std::string shipCatalogError;
		if (!ly::content::ShipContentCatalog::LoadFromFile(shipsPath, &shipCatalogError))
		{
			return Fail(("Ship catalog failed to load: " + shipCatalogError).c_str()) ? 0 : 1;
		}
		std::string enemyDefinitionCatalogError;
		if (!ly::content::EnemyContentCatalog::LoadFromFiles(
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_definitions.json",
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_behavior_profiles.json",
			&enemyDefinitionCatalogError))
		{
			return Fail(("Enemy definition catalog failed to load: " + enemyDefinitionCatalogError).c_str()) ? 0 : 1;
		}
		if (ly::content::EnemyContentCatalog::GetDefinitions().size() != 3 ||
			!ly::content::EnemyContentCatalog::FindById(ly::EnemyIds::ApproachGunnerBasic))
		{
			return Fail("Enemy definition catalog did not resolve the shipped generic enemies") ? 0 : 1;
		}

		const auto validateCombinedFixture = [&](const char* suffix, const char* combatJson, const char* behaviorJson, bool expected, const char* failureText)
		{
			const std::filesystem::path fixtureCombatPath = std::filesystem::temp_directory_path() / (std::string{ "lightyears_combined_" } + suffix + "_combat.json");
			const std::filesystem::path fixtureDefinitionsPath = std::filesystem::temp_directory_path() / (std::string{ "lightyears_combined_" } + suffix + "_definitions.json");
			const std::filesystem::path fixtureBehaviorPath = std::filesystem::temp_directory_path() / (std::string{ "lightyears_combined_" } + suffix + "_behavior.json");
			{
				std::ofstream file{ fixtureCombatPath }; file << combatJson;
				std::ofstream definitions{ fixtureDefinitionsPath };
				definitions << R"({"schemaVersion":1,"enemies":[{"id":"Enemy.Test.Combined","shipId":"Ship.Enemy.ApproachGunner.Basic","combatProfileId":"EnemyCombat.Test.Combined","behaviorProfileId":"EnemyBehavior.Test.Combined"}]})";
				std::ofstream behavior{ fixtureBehaviorPath }; behavior << behaviorJson;
			}
			std::string fixtureError;
			const bool combatLoaded = ly::content::EnemyCombatProfileCatalog::LoadFromFile(fixtureCombatPath, &fixtureError);
			const bool combinedLoaded = combatLoaded && ly::content::EnemyContentCatalog::LoadFromFiles(fixtureDefinitionsPath, fixtureBehaviorPath, &fixtureError);
			std::filesystem::remove(fixtureCombatPath);
			std::filesystem::remove(fixtureDefinitionsPath);
			std::filesystem::remove(fixtureBehaviorPath);
			ly::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath);
			ly::content::EnemyContentCatalog::LoadFromFiles(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_definitions.json",
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_behavior_profiles.json");
			if (combinedLoaded != expected)
			{
				return Fail(failureText) ? 0 : 1;
			}
			return 0;
		};
		const char* combinedBehaviorHeader = R"("schemaVersion":1,"profiles":[{"id":"EnemyBehavior.Test.Combined","targetSearchRange":2000.0,"targetRefreshInterval":0.2,"desiredDistance":700.0,"minimumDistance":400.0,"maximumDistance":0.0,"movementMode":"Approach","slotRules":)";
		if (validateCombinedFixture("weapon_range", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","weapons":[{"weaponId":"Weapon.Projectile.EnemyTwinBladeScatter.Basic","slot":"PrimaryFire"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"PrimaryFire","inputMode":"Hold","requiresTarget":true,"minimumRange":0.0,"maximumRange":501.0}]}]})").c_str(), false, "Combined validation accepted a behavior weapon range above Common.Range") != 0)
			return 1;
		if (validateCombinedFixture("missing_weapon_rule", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","weapons":[{"weaponId":"Weapon.Projectile.EnemyVanguardPulse.Basic","slot":"PrimaryFire"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + "[]}]}" ).c_str(), false, "Combined validation accepted an active weapon without a behavior rule") != 0)
			return 1;
		if (validateCombinedFixture("missing_ability_rule", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","abilities":[{"abilityId":"Ability.Movement.Dash.Basic","slot":"Ability1"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + "[]}]}" ).c_str(), false, "Combined validation accepted an active ability without a behavior rule") != 0)
			return 1;
		if (validateCombinedFixture("contact_active", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","weapons":[{"weaponId":"Weapon.Projectile.EnemyVanguardPulse.Basic","slot":"PrimaryFire"}],"allowContactDamageOnly":true}]})", (std::string{ "{" } + combinedBehaviorHeader + "[]}]}" ).c_str(), false, "Contact-only flag exempted an active binding without a behavior rule") != 0)
			return 1;
		if (validateCombinedFixture("contact_active_valid", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","weapons":[{"weaponId":"Weapon.Projectile.EnemyVanguardPulse.Basic","slot":"PrimaryFire"}],"allowContactDamageOnly":true}]})", (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"PrimaryFire","inputMode":"Hold","requiresTarget":true,"minimumRange":400.0,"maximumRange":700.0}]}]})").c_str(), true, "Contact-only permission incorrectly rejected a valid active binding") != 0)
			return 1;
		if (validateCombinedFixture("ability_only", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","abilities":[{"abilityId":"Ability.Movement.Dash.Basic","slot":"Ability1"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"Ability1","inputMode":"Pulse","requiresTarget":false}]}]})").c_str(), true, "Combined validation rejected a valid ability-only loadout") != 0)
			return 1;
		if (validateCombinedFixture("unsupported_policy", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","abilities":[{"abilityId":"Ability.Defense.DirectionalBarrier.Basic","slot":"Ability1"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"Ability1","inputMode":"Hold","requiresTarget":false}]}]})").c_str(), false, "Combined validation accepted an unsupported ability activation policy") != 0)
			return 1;
		const char* coveredCombat = R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","powerScalingPolicy":"Allowed","weapons":[{"weaponId":"Weapon.Projectile.EnemyVanguardPulse.Basic","slot":"PrimaryFire"},{"weaponId":"Weapon.Projectile.FighterRapidLaser.Basic","slot":"Ability1"}]}]})";
		if (validateCombinedFixture("coverage_accept", coveredCombat, (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"PrimaryFire","inputMode":"Hold","requiresTarget":true,"minimumRange":400.0,"maximumRange":500.0},{"slot":"Ability1","inputMode":"Hold","requiresTarget":true,"minimumRange":500.0,"maximumRange":700.0}]}]})").c_str(), true, "Combined validation rejected attack rules that cover the movement band") != 0)
			return 1;
		if (validateCombinedFixture("coverage_gap", coveredCombat, (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"PrimaryFire","inputMode":"Hold","requiresTarget":true,"minimumRange":400.0,"maximumRange":500.0},{"slot":"Ability1","inputMode":"Hold","requiresTarget":true,"minimumRange":600.0,"maximumRange":700.0}]}]})").c_str(), false, "Combined validation accepted a gap in the movement band") != 0)
			return 1;

		const ShipDefinition* approachShip = ly::content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
		const ShipDefinition* strafeShip = ly::content::ShipContentCatalog::FindById("Ship.Enemy.StrafeSkirmisher.Basic");
		const ShipDefinition* rangeShip = ly::content::ShipContentCatalog::FindById("Ship.Enemy.RangeKeeper.Basic");
		const PrimaryWeaponDefinition* approachWeapon = ly::content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyVanguardPulse.Basic");
		const PrimaryWeaponDefinition* strafeWeapon = ly::content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyTwinBladeScatter.Basic");
		const PrimaryWeaponDefinition* rangeWeapon = ly::content::WeaponContentCatalog::FindById("Weapon.Wave.EnemyHexagonCryoPulse.Basic");
		const ly::EnemyBehaviorProfile* approachBehavior = ly::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.ApproachGunner.Basic");
		const ly::EnemyBehaviorProfile* strafeBehavior = ly::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.StrafeSkirmisher.Basic");
		const ly::EnemyBehaviorProfile* rangeBehavior = ly::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.RangeKeeper.Basic");
		auto hasNonZeroOwnerAttribute = [](const ShipDefinition* ship, const sas::AttributeId& attributeId)
		{
			return ship && std::any_of(ship->baseOwnerAttributes.begin(), ship->baseOwnerAttributes.end(),
				[&](const OwnerAttributeBaseEntry& entry) { return entry.attributeId == attributeId && !NearlyEqual(entry.baseValue, 0.f); });
		};
		const bool enemyPowerScalingIsDisabled = approachShip && strafeShip && rangeShip &&
			NearlyEqual(approachShip->energyAttributes.baseEnergyPower, 0.f) && NearlyEqual(strafeShip->energyAttributes.baseEnergyPower, 0.f) &&
			NearlyEqual(rangeShip->energyAttributes.baseEnergyPower, 0.f) &&
			!hasNonZeroOwnerAttribute(approachShip, ly::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(strafeShip, ly::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(rangeShip, ly::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(approachShip, ly::OwnerAttributeIds::EnergyPower) &&
			!hasNonZeroOwnerAttribute(strafeShip, ly::OwnerAttributeIds::EnergyPower) &&
			!hasNonZeroOwnerAttribute(rangeShip, ly::OwnerAttributeIds::EnergyPower);
		const bool enemyStatsMatchRoles = approachShip && strafeShip && rangeShip &&
			NearlyEqual(approachShip->health, 60.f) && NearlyEqual(strafeShip->health, 60.f) && NearlyEqual(rangeShip->health, 100.f) &&
			NearlyEqual(approachShip->speed.x, 0.f) && NearlyEqual(approachShip->speed.y, 0.f) &&
			NearlyEqual(strafeShip->speed.x, 0.f) && NearlyEqual(strafeShip->speed.y, 0.f) &&
			NearlyEqual(rangeShip->speed.x, 0.f) && NearlyEqual(rangeShip->speed.y, 0.f) &&
			NearlyEqual(approachShip->collisionDamage, 50.f) && NearlyEqual(strafeShip->collisionDamage, 50.f) && NearlyEqual(rangeShip->collisionDamage, 60.f) &&
			NearlyEqual(approachShip->movementAttributes.forwardThrust.currentValue, 420.f) &&
			NearlyEqual(approachShip->movementAttributes.reverseThrust.currentValue, 110.f) &&
			NearlyEqual(approachShip->movementAttributes.strafeThrust.currentValue, 120.f) &&
			NearlyEqual(approachShip->movementAttributes.maxSpeed.currentValue, 420.f) &&
			NearlyEqual(strafeShip->movementAttributes.forwardThrust.currentValue, 320.f) &&
			NearlyEqual(strafeShip->movementAttributes.reverseThrust.currentValue, 220.f) &&
			NearlyEqual(strafeShip->movementAttributes.strafeThrust.currentValue, 300.f) &&
			NearlyEqual(strafeShip->movementAttributes.maxSpeed.currentValue, 460.f) &&
			NearlyEqual(rangeShip->movementAttributes.forwardThrust.currentValue, 240.f) &&
			NearlyEqual(rangeShip->movementAttributes.reverseThrust.currentValue, 300.f) &&
			NearlyEqual(rangeShip->movementAttributes.strafeThrust.currentValue, 180.f) &&
			NearlyEqual(rangeShip->movementAttributes.maxSpeed.currentValue, 360.f);
		const bool behaviorMatchesRoles = approachBehavior && strafeBehavior && rangeBehavior &&
			approachBehavior->movementMode == ly::EnemyMovementMode::Approach &&
			strafeBehavior->movementMode == ly::EnemyMovementMode::Strafe &&
			rangeBehavior->movementMode == ly::EnemyMovementMode::HoldRange &&
			NearlyEqual(approachBehavior->desiredDistance, 700.f) && NearlyEqual(approachBehavior->minimumDistance, 400.f) &&
			NearlyEqual(strafeBehavior->minimumDistance, 300.f) && NearlyEqual(strafeBehavior->maximumDistance, 450.f) &&
			NearlyEqual(rangeBehavior->minimumDistance, 750.f) && NearlyEqual(rangeBehavior->maximumDistance, 1050.f) &&
			NearlyEqual(approachBehavior->slotRules.front().maximumRange, 1200.f) &&
			NearlyEqual(strafeBehavior->slotRules.front().maximumRange, 500.f) &&
			NearlyEqual(rangeBehavior->slotRules.front().maximumRange, 1100.f);
		const bool weaponsMatchRoles = approachWeapon && strafeWeapon && rangeWeapon &&
			approachWeapon->weaponType == PrimaryWeaponType::ProjectileStandard &&
			strafeWeapon->weaponType == PrimaryWeaponType::ProjectileShotgun &&
			rangeWeapon->weaponType == PrimaryWeaponType::WaveExpanding &&
			approachWeapon->damageTags == ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Energy } &&
			strafeWeapon->damageTags == ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Kinetic } &&
			rangeWeapon->damageTags == ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Cryo } &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, ly::CommonAttributeIds::Damage)->baseValue, 15.f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, ly::CommonAttributeIds::Damage)->baseValue, 8.f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, ly::CommonAttributeIds::Damage)->baseValue, 10.f) &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, ly::CommonAttributeIds::FireRate)->baseValue, 1.2f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, ly::CommonAttributeIds::FireRate)->baseValue, 0.8f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, ly::CommonAttributeIds::FireRate)->baseValue, 0.6f) &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, ly::CommonAttributeIds::Range)->baseValue, 1200.f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, ly::CommonAttributeIds::Range)->baseValue, 500.f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, ly::CommonAttributeIds::Range)->baseValue, 1200.f);
		const auto hasMatchingRule = [](const ly::EnemyCombatProfile* combat, const ly::EnemyBehaviorProfile* behavior)
		{
			if (!combat || !behavior || combat->weapons.size() + combat->abilities.size() != behavior->slotRules.size()) return false;
			for (const ly::EnemyWeaponBinding& binding : combat->weapons)
				if (std::none_of(behavior->slotRules.begin(), behavior->slotRules.end(), [&](const ly::EnemySlotDecisionRule& rule) { return rule.slot == binding.slot; })) return false;
			for (const ly::EnemyAbilityBinding& binding : combat->abilities)
				if (std::none_of(behavior->slotRules.begin(), behavior->slotRules.end(), [&](const ly::EnemySlotDecisionRule& rule) { return rule.slot == binding.slot; })) return false;
			return true;
		};
		const ly::EnemyCombatProfile* approachCombat = ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
		const ly::EnemyCombatProfile* strafeCombat = ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.StrafeSkirmisher.Basic");
		const ly::EnemyCombatProfile* rangeCombat = ly::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.RangeKeeper.Basic");
		if (!enemyPowerScalingIsDisabled || !enemyStatsMatchRoles || !behaviorMatchesRoles || !weaponsMatchRoles ||
			!hasMatchingRule(approachCombat, approachBehavior) || !hasMatchingRule(strafeCombat, strafeBehavior) || !hasMatchingRule(rangeCombat, rangeBehavior))
		{
			return Fail("Shipped enemy ship, combat, behavior, or weapon profiles do not match their intended roles") ? 0 : 1;
		}

		const std::filesystem::path testEnemyDefinitionsPath =
			std::filesystem::temp_directory_path() / "lightyears_enemy_slot_test_definitions.json";
		const std::filesystem::path testEnemyBehaviorPath =
			std::filesystem::temp_directory_path() / "lightyears_enemy_slot_test_behavior.json";
		{
			std::ofstream definitionsFile{ testEnemyDefinitionsPath };
			definitionsFile << R"({
  "schemaVersion": 1,
  "enemies": [
    {
      "id": "Enemy.Test.SlotRules",
      "shipId": "Ship.Enemy.ApproachGunner.Basic",
      "combatProfileId": "EnemyCombat.ApproachGunner.Basic",
      "behaviorProfileId": "EnemyBehavior.Test.SlotRules"
    }
  ]
})";
		}
		{
			std::ofstream behaviorFile{ testEnemyBehaviorPath };
			behaviorFile << R"({
  "schemaVersion": 1,
  "profiles": [
    {
      "id": "EnemyBehavior.Test.SlotRules",
      "targetSearchRange": 1000.0,
      "targetRefreshInterval": 0.2,
      "desiredDistance": 400.0,
      "minimumDistance": 200.0,
      "maximumDistance": 0.0,
      "movementMode": "Approach",
      "fireRange": 500.0,
      "fireConeThreshold": 0.9
    }
  ]
})";
		}
		std::string legacyBehaviorError;
		const bool legacyBehaviorLoaded = ly::content::EnemyContentCatalog::LoadFromFiles(
			testEnemyDefinitionsPath, testEnemyBehaviorPath, &legacyBehaviorError);
		if (legacyBehaviorLoaded ||
			(legacyBehaviorError.find("fireRange") == std::string::npos &&
			 legacyBehaviorError.find("fireConeThreshold") == std::string::npos))
		{
			std::filesystem::remove(testEnemyDefinitionsPath);
			std::filesystem::remove(testEnemyBehaviorPath);
			return Fail("Legacy fireRange behavior fields were not rejected") ? 0 : 1;
		}
		{
			std::ofstream behaviorFile{ testEnemyBehaviorPath };
			behaviorFile << R"({
  "schemaVersion": 1,
  "profiles": [
    {
      "id": "EnemyBehavior.Test.SlotRules",
      "targetSearchRange": 1000.0,
      "targetRefreshInterval": 0.2,
      "desiredDistance": 400.0,
      "minimumDistance": 200.0,
      "maximumDistance": 0.0,
      "movementMode": "Approach",
      "slotRules": [
        {
          "slot": "Ability1",
          "inputMode": "Pulse",
          "requiresTarget": false
        }
      ]
    }
  ]
})";
		}
		std::string unboundRuleError;
		const bool unboundRuleLoaded = ly::content::EnemyContentCatalog::LoadFromFiles(
			testEnemyDefinitionsPath, testEnemyBehaviorPath, &unboundRuleError);
		std::filesystem::remove(testEnemyDefinitionsPath);
		std::filesystem::remove(testEnemyBehaviorPath);
		if (unboundRuleLoaded || unboundRuleError.find("active combat binding") == std::string::npos)
			return Fail("Behavior slot rule without an active combat binding was accepted") ? 0 : 1;

		std::cout << "[PASS] All Enemy Combat Profile content tests passed successfully!\n";
		return 0;
	}
}

int main()
{
	const int enemyProfileResult = RunEnemyCombatProfileContentTests();
	if (enemyProfileResult != 0)
	{
		return enemyProfileResult;
	}
	const std::filesystem::path abilityPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/abilities.json";
	const ly::List<const ly::GameAbilityDefinition*>& fallbackAbilities =
		AbilityData::GetBuiltinShippedAbilityDefinitions();
	const ly::List<const ly::AbilityActorDefinition*>& fallbackAbilityActors =
		AbilityData::GetBuiltinAbilityActorDefinitions();
	const ly::content::AbilityLoader::Result loadedAbilities =
		ly::content::AbilityLoader::LoadFromFile(
			abilityPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	if (!loadedAbilities.Succeeded())
	{
		return Fail(loadedAbilities.error.c_str()) ? 0 : 1;
	}
	if (loadedAbilities.definitions.size() != 55)
	{
		return Fail("Ability JSON catalog did not load the expected pilot definitions") ? 0 : 1;
	}
	std::size_t actorDefinitionCount = 0;
	for (const auto& definition : loadedAbilities.definitions)
	{
		actorDefinitionCount += definition.actorDefinitions.size();
	}
	if (actorDefinitionCount != 33)
	{
		return Fail("Ability actor JSON catalog did not load the expected actor definitions") ? 0 : 1;
	}

	const ly::content::AbilityLoader::LoadedDefinition* railBurstLoaded = nullptr;
	for (const ly::content::AbilityLoader::LoadedDefinition& loaded : loadedAbilities.definitions)
	{
		if (loaded.id == "Ability.Offense.RailBurst.Basic")
		{
			railBurstLoaded = &loaded;
			break;
		}
	}
	if (!railBurstLoaded || railBurstLoaded->actorDefinitions.size() != 1)
	{
		return Fail("Rail Burst JSON definition or projectile actor was not loaded") ? 0 : 1;
	}
	if (sas::FindAttribute(
		railBurstLoaded->actorDefinitions.front().attributes,
		ly::CommonAttributeIds::PierceDamageLoss
	))
	{
		return Fail("Rail Burst still declares removed damage falloff") ? 0 : 1;
	}

	const auto astralSurgeIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.AstralSurge.Basic";
		}
	);
	if (astralSurgeIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(astralSurgeIt->definition.cooldown, 16.f) ||
		!NearlyEqual(astralSurgeIt->definition.duration, 1.f) ||
		astralSurgeIt->definition.maxCharges != 1 ||
		astralSurgeIt->definition.scalingRules.size() != 1 ||
		astralSurgeIt->definition.scalingRules.front().sourceAttributeId !=
			ly::OwnerAttributeIds::EnergyPower ||
		!NearlyEqual(astralSurgeIt->definition.scalingRules.front().coefficient, 0.45f) ||
		astralSurgeIt->definition.levelProgression.size() != 14 ||
		astralSurgeIt->actorDefinitions.size() != 1 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				astralSurgeIt->actorDefinitions.front().attributes,
				ly::AreaAttributeIds::Width,
				0.f
			),
			280.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				astralSurgeIt->actorDefinitions.front().attributes,
				ly::CommonAttributeIds::PierceDamageLoss,
				0.f
			),
			0.05f
		))
	{
		return Fail("Astral Surge JSON profile is invalid") ? 0 : 1;
	}

	const auto crescentReaverIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.CrescentReaver.Basic";
		}
	);
	const ly::SpawnActorAction* crescentSpawn = nullptr;
	if (crescentReaverIt != loadedAbilities.definitions.end())
	{
		for (const ly::AbilityActionSpec& action : crescentReaverIt->definition.actions)
		{
			if (const auto* spawn = std::get_if<ly::SpawnActorAction>(&action.action))
			{
				crescentSpawn = spawn;
				break;
			}
		}
	}
	if (crescentReaverIt == loadedAbilities.definitions.end() ||
		crescentReaverIt->definition.scalingRules.size() != 2 ||
		crescentReaverIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Kinetic } ||
		!crescentSpawn ||
		crescentSpawn->spawnPolicy != sas::AbilitySpawnPolicy::OwnerForward ||
		crescentSpawn->directionPolicy != sas::AbilityDirectionPolicy::MouseWorld ||
		crescentReaverIt->actorDefinitions.size() != 1 ||
		!sas::FindAttribute(
			crescentReaverIt->actorDefinitions.front().attributes,
			AbilityData::CrescentReaver::Actor::Projectile::BounceCount
		))
	{
		return Fail("Crescent Reaver JSON profile or cursor direction contract is invalid") ? 0 : 1;
	}

	const auto energySpearIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Movement.EnergySpear.Basic";
		}
	);
	if (energySpearIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(energySpearIt->definition.cooldown, 8.f) ||
		!NearlyEqual(energySpearIt->definition.duration, 1.5f) ||
		energySpearIt->definition.attributes.size() != 10 ||
		energySpearIt->definition.levelProgression.size() != 14 ||
		energySpearIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Energy } ||
		!sas::FindAttribute(
			energySpearIt->definition.attributes,
			ly::CommonAttributeIds::Radius
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				energySpearIt->definition.attributes,
				ly::CommonAttributeIds::Range,
				0.f
			),
			600.f
		))
	{
		return Fail("Energy Spear ability JSON profile is invalid") ? 0 : 1;
	}

	const auto frostMaelstromIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Control.FrostMaelstrom.Basic";
		}
	);
	if (frostMaelstromIt == loadedAbilities.definitions.end() ||
		frostMaelstromIt->definition.attributes.size() != 13 ||
		frostMaelstromIt->definition.levelProgression.size() != 14 ||
		!NearlyEqual(frostMaelstromIt->definition.duration, 6.f) ||
		frostMaelstromIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Cryo } ||
		frostMaelstromIt->actorDefinitions.size() != 1 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->actorDefinitions.front().attributes,
				ly::CommonAttributeIds::Duration,
				0.f
			),
			6.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::MaximumRadius,
				0.f
			),
			600.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::InwardForce,
				0.f
			),
			1500.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::OrbitalAngularSpeed,
				0.f
			),
			4.5f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio,
				0.f
			),
			0.42f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::TickInterval,
				0.f
			),
			0.25f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->definition.attributes,
				AbilityData::FrostMaelstrom::Attribute::CryoStacksPerTick,
				0.f
			),
			1.f
		))
	{
		return Fail("Frost Maelstrom JSON runtime and Cryo tick contract is invalid") ? 0 : 1;
	}

	const std::filesystem::path duplicateAbilityPath =
		std::filesystem::temp_directory_path() /
		"lightyears_duplicate_ability_test.json";
	{
		std::ofstream duplicateFile{ duplicateAbilityPath };
		duplicateFile << R"({
  "schemaVersion": 1,
  "abilities": [
    {
      "id": "Ability.Movement.Dash.Basic",
      "cooldown": 2.0,
      "duration": 0.24,
      "maxCharges": 1,
      "settings": { "baseDistance": 260.0, "cameraZoomOutRatio": 0.15 }
    },
    {
      "id": "Ability.Movement.Dash.Basic",
      "cooldown": 2.0,
      "duration": 0.24,
      "maxCharges": 1,
      "settings": { "baseDistance": 260.0, "cameraZoomOutRatio": 0.15 }
    }
  ]
})";
	}
	const ly::content::AbilityLoader::Result duplicateAbilities =
		ly::content::AbilityLoader::LoadFromFile(
			duplicateAbilityPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(duplicateAbilityPath);
	if (duplicateAbilities.Succeeded() ||
		duplicateAbilities.error.find("Duplicate ability ID") == std::string::npos)
	{
		return Fail("Ability loader accepted a duplicate ability ID") ? 0 : 1;
	}

	const std::filesystem::path malformedAbilityIdPath =
		std::filesystem::temp_directory_path() /
		"lightyears_malformed_ability_id_test.json";
	{
		std::ofstream malformedAbilityIdFile{ malformedAbilityIdPath };
		malformedAbilityIdFile << R"({
  "schemaVersion": 1,
  "abilities": [{
    "id": "Ability.Offense.Rocket",
    "cooldown": 1.0,
    "duration": 0.1,
    "maxCharges": 1,
    "settings": {}
  }]
})";
	}
	const ly::content::AbilityLoader::Result malformedAbilityId =
		ly::content::AbilityLoader::LoadFromFile(
			malformedAbilityIdPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(malformedAbilityIdPath);
	if (malformedAbilityId.Succeeded() ||
		malformedAbilityId.error.find("Invalid ability ID") == std::string::npos)
	{
		return Fail("Ability loader accepted an ID without a variant segment") ? 0 : 1;
	}

	const std::filesystem::path abilityVariantPath =
		std::filesystem::temp_directory_path() /
		"lightyears_ability_variant_test.json";
	{
		std::ofstream variantFile{ abilityVariantPath };
		variantFile << R"({
  "schemaVersion": 1,
  "abilities": [
    {
      "id": "Ability.Movement.Dash.Basic",
      "cooldown": 2.0,
      "duration": 0.24,
      "maxCharges": 1,
      "settings": { "baseDistance": 333.0, "cameraZoomOutRatio": 0.15 }
    },
    {
      "id": "Ability.Movement.Dash.Variant",
      "baseId": "Ability.Movement.Dash.Basic",
      "cooldown": 1.5,
      "duration": 0.20,
      "maxCharges": 1,
      "settings": { "cameraZoomOutRatio": 0.20 }
    }
  ]
})";
	}
	const ly::content::AbilityLoader::Result abilityVariant =
		ly::content::AbilityLoader::LoadFromFile(
			abilityVariantPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(abilityVariantPath);
	if (!abilityVariant.Succeeded() || abilityVariant.definitions.size() != 2 ||
		abilityVariant.definitions.back().definition.abilityId !=
			"Ability.Movement.Dash.Variant" ||
		!NearlyEqual(
			abilityVariant.definitions.back().numericSettings.at("baseDistance"),
			333.f
		) ||
		!NearlyEqual(
			abilityVariant.definitions.back().numericSettings.at("cameraZoomOutRatio"),
			0.20f
		))
	{
		return Fail("Ability loader could not inherit and override a value-only ability variant") ? 0 : 1;
	}

	const std::filesystem::path abilityScopedAttributesPath =
		std::filesystem::temp_directory_path() /
		"lightyears_ability_scoped_attributes_test.json";
	{
		std::ofstream attributesFile{ abilityScopedAttributesPath };
		attributesFile << R"({
  "schemaVersion": 1,
  "abilities": [{
    "id": "Ability.Movement.Dash.Basic",
    "cooldown": 2.0,
    "duration": 0.24,
    "maxCharges": 1,
    "settings": { "baseDistance": 260.0, "cameraZoomOutRatio": 0.15 },
    "attributes": [
      { "id": "Common.ProjectileCount", "baseValue": 8.0, "minValue": 1.0 },
      { "id": "Ability.Movement.Dash.TestValue", "baseValue": 2.0 }
    ]
  }]
})";
	}
	const ly::content::AbilityLoader::Result abilityScopedAttributes =
		ly::content::AbilityLoader::LoadFromFile(
			abilityScopedAttributesPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(abilityScopedAttributesPath);
	if (!abilityScopedAttributes.Succeeded() ||
		abilityScopedAttributes.definitions.size() != 1 ||
		sas::FindAttributeValue(
			abilityScopedAttributes.definitions.front().definition.attributes,
			ly::CommonAttributeIds::ProjectileCount
		) != 8.f ||
		sas::FindAttributeValue(
			abilityScopedAttributes.definitions.front().definition.attributes,
			sas::AttributeId{ "Ability.Movement.Dash.TestValue" }
		) != 2.f)
	{
		return Fail("Ability loader did not materialize ability-scoped attributes") ? 0 : 1;
	}

	const std::filesystem::path missingAbilityNumbersPath =
		std::filesystem::temp_directory_path() /
		"lightyears_missing_ability_numbers_test.json";
	{
		std::ofstream missingAbilityNumbersFile{ missingAbilityNumbersPath };
		missingAbilityNumbersFile << R"({
  "schemaVersion": 1,
  "abilities": [{
    "id": "Ability.Movement.Dash.Basic",
    "cooldown": 1.0,
    "settings": { "baseDistance": 300.0, "cameraZoomOutRatio": 0.10 }
  }]
})";
	}
	const ly::content::AbilityLoader::Result missingAbilityNumbers =
		ly::content::AbilityLoader::LoadFromFile(
			missingAbilityNumbersPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(missingAbilityNumbersPath);
	if (missingAbilityNumbers.Succeeded() ||
		missingAbilityNumbers.error.find("requires numeric JSON field") == std::string::npos)
	{
		return Fail("Ability loader accepted missing required JSON numeric fields") ? 0 : 1;
	}
	const auto& dash = loadedAbilities.definitions.front();
	if (dash.id != "Ability.Movement.Dash.Basic" ||
		!NearlyEqual(dash.definition.cooldown, 2.f) ||
		!NearlyEqual(dash.definition.duration, 0.24f) ||
		dash.definition.maxCharges != 1 ||
		dash.definition.levelProgression.size() != 4 ||
		dash.definition.levelUpgradeScrapCosts != ly::List<unsigned int>{ 40u, 50u, 65u, 80u } ||
		dash.numericSettings.at("baseDistance") != 260.f ||
		dash.numericSettings.at("cameraZoomOutRatio") != 0.15f)
	{
		return Fail("Dash ability JSON profile is invalid") ? 0 : 1;
	}
	const auto shieldIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Defense.Shield.Basic";
		}
	);
	if (shieldIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(shieldIt->definition.cooldown, 8.f) ||
		!NearlyEqual(shieldIt->definition.duration, 5.f) ||
		shieldIt->definition.maxCharges != 1 ||
		shieldIt->definition.scalingRules.size() != 2 ||
		shieldIt->definition.scalingRules.front().targetAttributeId !=
			BarrierEffectSchema::Capacity ||
		!NearlyEqual(shieldIt->definition.scalingRules.front().coefficient, 0.2f) ||
		shieldIt->definition.effectSpecs.size() != 2 ||
		!shieldIt->definition.effectSpecs.front().useAbilityDuration ||
		!NearlyEqual(
			sas::FindAttributeValue(
				shieldIt->definition.effectSpecs.front().attributes,
				BarrierEffectSchema::Capacity
			),
			30.f
		))
	{
		return Fail("Shield ability JSON profile is invalid") ? 0 : 1;
	}
	const auto sunBeamIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.SunBeam.Strike.Basic";
		}
	);
	if (sunBeamIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(sunBeamIt->definition.cooldown, 1.f) ||
		sunBeamIt->definition.levelProgression.size() != 4 ||
		sunBeamIt->definition.levelUpgradeScrapCosts != ly::List<unsigned int>{ 40u, 50u, 65u, 80u })
	{
		return Fail("Sun Beam ability JSON profile is invalid") ? 0 : 1;
	}
	const auto rocketIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.Rocket.Basic";
		}
	);
	if (rocketIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(rocketIt->definition.cooldown, 3.f) ||
		rocketIt->definition.levelProgression.size() != 14 ||
		rocketIt->definition.levelUpgradeScrapCosts.size() != 14)
	{
		return Fail("Rocket ability JSON profile is invalid") ? 0 : 1;
	}
	const auto gravityIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Control.GravityAnomaly.Basic";
		}
	);
	if (gravityIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(gravityIt->definition.cooldown, 8.f) ||
		gravityIt->definition.levelProgression.size() != 14 ||
		gravityIt->definition.levelProgression.front().attributeModifiers.size() != 7 ||
		gravityIt->definition.levelUpgradeScrapCosts.size() != 14 ||
		gravityIt->actorDefinitions.size() != 2 ||
		gravityIt->actorDefinitions.front().attributes.size() != 7 ||
		gravityIt->actorDefinitions.back().attributes.size() != 5 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				gravityIt->actorDefinitions.front().attributes,
				ly::CommonAttributeIds::Radius
			),
			320.f
		))
	{
		return Fail("Gravity Anomaly ability JSON profile is invalid") ? 0 : 1;
	}
	const auto infernoIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.InfernoSpray.Basic";
		}
	);
	if (infernoIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(infernoIt->definition.cooldown, 3.f) ||
		!NearlyEqual(infernoIt->definition.duration, 3.f) ||
		infernoIt->definition.maxCharges != 1)
	{
		return Fail("Inferno Spray ability JSON profile is invalid") ? 0 : 1;
	}
	const auto overdriveIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.OverdriveCore.Basic";
		}
	);
	if (overdriveIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(overdriveIt->definition.cooldown, 1.f) ||
		!NearlyEqual(overdriveIt->definition.duration, 6.f) ||
		overdriveIt->definition.attributes.size() != 7 ||
		overdriveIt->actorDefinitions.size() != 1 ||
		overdriveIt->actorDefinitions.front().presentationProfileId !=
			ly::OverdriveCorePresentationIds::ProjectileBasic)
	{
		return Fail("Overdrive Core ability JSON profile is invalid") ? 0 : 1;
	}
	const auto nullPulseIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Control.NullPulse.Basic";
		}
	);
	if (nullPulseIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(nullPulseIt->definition.cooldown, 11.f) ||
		nullPulseIt->definition.duration != 0.f ||
		nullPulseIt->definition.maxCharges != 1 ||
		nullPulseIt->definition.attributes.size() != 7 ||
		nullPulseIt->definition.levelProgression.size() != 14 ||
		nullPulseIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Energy })
	{
		return Fail("Null Pulse ability JSON profile is invalid") ? 0 : 1;
	}
	const auto phaseDriftIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Movement.PhaseDrift.Basic";
		}
	);
	if (phaseDriftIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(phaseDriftIt->definition.cooldown, 14.f) ||
		!NearlyEqual(phaseDriftIt->definition.duration, 6.f) ||
		phaseDriftIt->definition.maxCharges != 1 ||
		phaseDriftIt->definition.attributes.size() != 8 ||
		phaseDriftIt->definition.levelProgression.size() != 14)
	{
		return Fail("Phase Drift ability JSON profile is invalid") ? 0 : 1;
	}
	const auto shieldHarvestIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Defense.ShieldHarvest.Basic";
		}
	);
	if (shieldHarvestIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(shieldHarvestIt->definition.cooldown, 14.f) ||
		!NearlyEqual(shieldHarvestIt->definition.duration, 1.5f) ||
		shieldHarvestIt->definition.maxCharges != 1 ||
		shieldHarvestIt->definition.attributes.size() != 4 ||
		shieldHarvestIt->definition.levelProgression.size() != 14 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				shieldHarvestIt->definition.attributes,
				ly::CommonAttributeIds::Radius
			),
			700.f
		))
	{
		return Fail("Shield Harvest ability JSON profile is invalid") ? 0 : 1;
	}
	const auto hullShockIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.HullShock.Basic";
		}
	);
	if (hullShockIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(hullShockIt->definition.cooldown, 12.f) ||
		!NearlyEqual(hullShockIt->definition.duration, 2.f) ||
		hullShockIt->definition.maxCharges != 1 ||
		hullShockIt->definition.attributes.size() != 9 ||
		hullShockIt->definition.scalingRules.size() != 1 ||
		hullShockIt->definition.levelProgression.size() != 14 ||
		hullShockIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Electric } ||
		!NearlyEqual(
			sas::FindAttributeValue(
				hullShockIt->definition.attributes,
				ly::CommonAttributeIds::Radius
			),
			600.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				hullShockIt->definition.attributes,
				ly::CommonAttributeIds::Damage
			),
			30.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				hullShockIt->definition.attributes,
				AbilityData::HullShock::Attribute::MinimumChargeRadius
			),
			300.f
		))
	{
		return Fail("Hull Shock ability JSON profile is invalid") ? 0 : 1;
	}
	const auto orbitalDronesIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.OrbitalDrones.Basic";
		}
	);
	if (orbitalDronesIt == loadedAbilities.definitions.end() ||
		orbitalDronesIt->definition.slot != sas::AbilitySlot::Ability4 ||
		orbitalDronesIt->definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		orbitalDronesIt->definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		!NearlyEqual(orbitalDronesIt->definition.cooldown, 12.f) ||
		!NearlyEqual(orbitalDronesIt->definition.duration, 6.f) ||
		orbitalDronesIt->definition.maxCharges != 1 ||
		orbitalDronesIt->definition.abilityTags !=
			ly::List<ly::GameplayTag>{
				ly::GameplayTags::Ability::Offense,
				ly::GameplayTags::Ability::Family::OrbitalDrones
			} ||
		orbitalDronesIt->definition.attributes.size() != 8 ||
		sas::FindAttributeValue(
			orbitalDronesIt->definition.attributes,
			ly::CommonAttributeIds::Radius,
			0.f
		) <= 0.f ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				ly::CommonAttributeIds::Damage,
				0.f
			),
			18.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::DroneCount,
				0.f
			),
			4.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::SameTargetHitCooldown,
				0.f
			),
			0.5f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
				0.f
			),
			2.5f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::ContactRadius,
				0.f
			),
			12.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::EnergyPowerReference,
				0.f
			),
			50.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				AbilityData::OrbitalDrones::Attribute::EnergyPowerDurationScale,
				0.f
			),
			0.02f
		) ||
		orbitalDronesIt->definition.scalingRules.size() != 1 ||
		orbitalDronesIt->definition.scalingRules.front().targetAttributeId !=
			ly::CommonAttributeIds::Damage ||
		orbitalDronesIt->definition.scalingRules.front().sourceAttributeId !=
			ly::OwnerAttributeIds::AttackPower ||
		orbitalDronesIt->definition.scalingRules.front().operation !=
			sas::AttributeModifierOperation::Add ||
		!NearlyEqual(orbitalDronesIt->definition.scalingRules.front().coefficient, 0.50f) ||
		orbitalDronesIt->definition.levelProgression.size() != 14 ||
		orbitalDronesIt->definition.levelUpgradeScrapCosts.size() != 14 ||
		orbitalDronesIt->definition.damageTags !=
			ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Kinetic })
	{
		return Fail("Orbital Drones ability JSON profile is invalid") ? 0 : 1;
	}
	for (const ly::AbilityLevelStep& step : orbitalDronesIt->definition.levelProgression)
	{
		bool hasDamageUpgrade = false;
		bool hasCooldownUpgrade = false;
		if (step.attributeModifiers.size() != 2)
		{
			return Fail("Orbital Drones level progression does not contain exactly two modifiers")
				? 0
				: 1;
		}
		for (const sas::AttributeModifier& modifier : step.attributeModifiers)
		{
			if (modifier.attributeId == ly::CommonAttributeIds::Damage &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(modifier.magnitude, 2.f))
			{
				hasDamageUpgrade = true;
			}
			if (modifier.attributeId == ly::CommonAttributeIds::Cooldown &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(modifier.magnitude, -0.25f))
			{
				hasCooldownUpgrade = true;
			}
		}
		if (!hasDamageUpgrade || !hasCooldownUpgrade)
		{
			return Fail("Orbital Drones level progression has an unexpected modifier") ? 0 : 1;
		}
	}

	const auto relayPrismIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const ly::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Utility.RelayPrism.Basic";
		}
	);
	if (relayPrismIt == loadedAbilities.definitions.end() ||
		relayPrismIt->definition.slot != sas::AbilitySlot::Ability2 ||
		relayPrismIt->definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		relayPrismIt->definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		!NearlyEqual(relayPrismIt->definition.cooldown, 10.f) ||
		!NearlyEqual(relayPrismIt->definition.duration, 4.f) ||
		relayPrismIt->definition.maxCharges != 1 ||
		relayPrismIt->definition.attributes.size() != 6 ||
		relayPrismIt->actorDefinitions.size() != 1 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				relayPrismIt->definition.attributes,
				ly::CommonAttributeIds::ProjectileCount,
				0.f
			),
			4.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				relayPrismIt->definition.attributes,
				AbilityData::RelayPrism::Attribute::DamageTransferRatio,
				0.f
			),
			0.15f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				relayPrismIt->actorDefinitions.front().attributes,
				ly::CommonAttributeIds::Radius,
				0.f
			),
			100.f
		) ||
		relayPrismIt->definition.levelProgression.size() != 14 ||
		relayPrismIt->definition.levelUpgradeScrapCosts.size() != 14)
	{
		return Fail("Relay Prism ability JSON profile is invalid") ? 0 : 1;
	}

	const std::filesystem::path weaponPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/weapons.json";

	const ly::content::WeaponLoader::Result loaded =
		ly::content::WeaponLoader::LoadFromFile(weaponPath);
	if (!loaded.Succeeded())
	{
		return Fail(loaded.error.c_str()) ? 0 : 1;
	}
	if (loaded.definitions.size() != 10)
	{
		return Fail("Weapon JSON catalog did not load exactly ten definitions") ? 0 : 1;
	}

	const char* expectedWeaponIds[] = {
		"Weapon.Projectile.FighterRapidLaser.Basic",
		"Weapon.Projectile.RapidShotgun.Basic",
		"Weapon.Projectile.DualKineticBlaster.Basic",
		"Weapon.Arc.ElectricLauncher.Basic",
		"Weapon.Beam.ContinuousHeatLaser.Basic",
		"Weapon.Wave.CryoProjector.Basic",
		"Weapon.Projectile.IroncladMinigun.Basic",
		"Weapon.Projectile.EnemyVanguardPulse.Basic",
		"Weapon.Projectile.EnemyTwinBladeScatter.Basic",
		"Weapon.Wave.EnemyHexagonCryoPulse.Basic"
	};
	for (const char* expectedId : expectedWeaponIds)
	{
		if (!FindWeapon(loaded.definitions, expectedId))
		{
			return Fail("Weapon JSON catalog is missing an expected weapon") ? 0 : 1;
		}
	}

	const PrimaryWeaponDefinition* basicLaser =
		FindWeapon(loaded.definitions, "Weapon.Projectile.FighterRapidLaser.Basic");
	const sas::GameplayAttribute* basicDamage = sas::FindAttribute(
		basicLaser->attributes,
		ly::CommonAttributeIds::Damage
	);
	if (basicLaser->weaponType != PrimaryWeaponType::ProjectileStandard ||
		!basicDamage ||
		!NearlyEqual(basicDamage->baseValue, 12.f) ||
		basicLaser->progressionProfile.ResolveLevelSteps().size() != 14 ||
		basicLaser->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Photonic })
	{
		return Fail("Basic rapid laser JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* shotgun =
		FindWeapon(loaded.definitions, "Weapon.Projectile.RapidShotgun.Basic");
	if (shotgun->weaponType != PrimaryWeaponType::ProjectileShotgun ||
		shotgun->muzzleDefinitions.size() != 1 ||
		shotgun->attributes.size() != 16 ||
		!sas::FindAttribute(shotgun->attributes, ly::DamageAttributeIds::BurnDuration) ||
		shotgun->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Thermal })
	{
		return Fail("Rapid shotgun JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* dual =
		FindWeapon(loaded.definitions, "Weapon.Projectile.DualKineticBlaster.Basic");
	if (dual->muzzleDefinitions.size() != 2 ||
		dual->progressionProfile.ResolveLevelSteps().size() != 3 ||
		dual->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Kinetic })
	{
		return Fail("Dual kinetic blaster JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* electric =
		FindWeapon(loaded.definitions, "Weapon.Arc.ElectricLauncher.Basic");
	if (electric->weaponType != PrimaryWeaponType::ArcElectric ||
		!sas::FindAttribute(
			electric->attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainCount
		) ||
		electric->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Electric })
	{
		return Fail("Electric arc launcher JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* beam =
		FindWeapon(loaded.definitions, "Weapon.Beam.ContinuousHeatLaser.Basic");
	if (beam->weaponType != PrimaryWeaponType::BeamContinuous ||
		beam->featureTypes != ly::List<PrimaryWeaponFeatureType>{ PrimaryWeaponFeatureType::Heat } ||
		beam->heatGainCurve.size() != 4 ||
		beam->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Energy })
	{
		return Fail("Continuous heat laser JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* cryo =
		FindWeapon(loaded.definitions, "Weapon.Wave.CryoProjector.Basic");
	if (cryo->weaponType != PrimaryWeaponType::WaveExpanding ||
		cryo->attributes.size() != 12 ||
		cryo->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Cryo })
	{
		return Fail("Cryo wave projector JSON profile is invalid") ? 0 : 1;
	}

	const std::filesystem::path shipPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/ships.json";
	const ShipDefinition shipPresentationBase{
		"",
		100.f,
		sf::Vector2f{ 350.f, 350.f },
		25.f,
		0.f,
		0,
		{},
		{}
	};
	const ly::content::ShipLoader::Result loadedShips =
		ly::content::ShipLoader::LoadFromFile(
			shipPath,
			shipPresentationBase
		);
	if (!loadedShips.Succeeded() || loadedShips.definitions.size() != 4 ||
		loadedShips.definitions.front().id != "Ship.Player.Fighter.Basic" ||
		loadedShips.definitions.front().definition.primaryWeaponId !=
			"Weapon.Projectile.FighterRapidLaser.Basic" ||
		!NearlyEqual(loadedShips.definitions.front().definition.health, 250.f))
	{
		return Fail("Ship JSON catalog did not load the player definition") ? 0 : 1;
	}

	const std::filesystem::path attachmentPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/attachments.json";
	const ly::content::AttachmentLoader::Result loadedAttachments =
		ly::content::AttachmentLoader::LoadFromFile(attachmentPath);
	if (!loadedAttachments.Succeeded() || loadedAttachments.definitions.size() != 1 ||
		loadedAttachments.definitions.front().attachmentId !=
			"Attachment.Thermal.Converter.Basic" ||
		loadedAttachments.definitions.front().eventRules.size() != 1)
	{
		return Fail("Attachment JSON catalog did not load the expected definition") ? 0 : 1;
	}

	const std::filesystem::path effectPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/effects.json";
	const ly::content::EffectLoader::Result loadedEffects =
		ly::content::EffectLoader::LoadFromFile(
			effectPath,
		EffectData::GetBuiltinGameplayEffectDefinitions()
	);
	const auto barrierEffectIt = std::find_if(
		loadedEffects.definitions.begin(),
		loadedEffects.definitions.end(),
		[](const ly::content::EffectLoader::LoadedDefinition& definition)
		{
			return definition.id == "Effect.Barrier.Basic";
		}
	);
	if (!loadedEffects.Succeeded())
	{
		return Fail(loadedEffects.error.c_str()) ? 0 : 1;
	}
	if (loadedEffects.definitions.size() != 19 ||
		barrierEffectIt == loadedEffects.definitions.end() ||
		!barrierEffectIt->definition.sourceParameterized ||
		!barrierEffectIt->definition.attributes.empty() ||
		!barrierEffectIt->definition.modifiers.empty() ||
		!NearlyEqual(barrierEffectIt->definition.duration, 0.f))
	{
		return Fail("Shield barrier effect JSON profile is invalid") ? 0 : 1;
	}
	const std::filesystem::path invalidEffectPath =
		std::filesystem::temp_directory_path() / "lightyears_invalid_effect_ownership.json";
	{
		std::ofstream invalidEffectFile{ invalidEffectPath };
		invalidEffectFile << R"({
  "schemaVersion": 1,
  "effects": [{
    "id": "Effect.Barrier.Basic",
    "durationPolicy": "Duration",
    "sourceParameterized": true,
    "duration": 5.0
  }]
})";
	}
	const ly::content::EffectLoader::Result invalidOwnedEffect =
		ly::content::EffectLoader::LoadFromFile(
			invalidEffectPath,
			EffectData::GetBuiltinGameplayEffectDefinitions()
		);
	std::filesystem::remove(invalidEffectPath);
	if (invalidOwnedEffect.Succeeded())
	{
		return Fail("Effect ownership lint accepted source-owned numeric data in effects.json") ? 0 : 1;
	}
	const std::filesystem::path invalidEffectTagsPath =
		std::filesystem::temp_directory_path() / "lightyears_invalid_effect_tags_test.json";
	{
		std::ofstream invalidEffectTagsFile{ invalidEffectTagsPath };
		invalidEffectTagsFile << R"({
  "schemaVersion": 1,
  "effects": [{
    "id": "Effect.Barrier.Basic",
    "durationPolicy": "Instant",
    "stackingPolicy": "None",
    "sourceParameterized": false,
    "grantedTags": ["Attribute.Owner.Health"]
  }]
})";
	}
	const ly::content::EffectLoader::Result invalidEffectTags =
		ly::content::EffectLoader::LoadFromFile(
			invalidEffectTagsPath,
			EffectData::GetBuiltinGameplayEffectDefinitions()
		);
	std::filesystem::remove(invalidEffectTagsPath);
	if (invalidEffectTags.Succeeded() ||
		invalidEffectTags.error.find("invalid granted tag") == std::string::npos)
	{
		return Fail("Effect loader accepted a tag from an unrelated domain") ? 0 : 1;
	}
	const std::filesystem::path malformedEffectIdPath =
		std::filesystem::temp_directory_path() / "lightyears_malformed_effect_id_test.json";
	{
		std::ofstream malformedEffectIdFile{ malformedEffectIdPath };
		malformedEffectIdFile << R"({
  "schemaVersion": 1,
  "effects": [{
    "id": "Effect.Barrier",
    "durationPolicy": "Instant",
    "stackingPolicy": "None",
    "sourceParameterized": false
  }]
})";
	}
	const ly::content::EffectLoader::Result malformedEffectId =
		ly::content::EffectLoader::LoadFromFile(
			malformedEffectIdPath,
			EffectData::GetBuiltinGameplayEffectDefinitions()
		);
	std::filesystem::remove(malformedEffectIdPath);
	if (malformedEffectId.Succeeded() ||
		malformedEffectId.error.find("Invalid gameplay effect ID") == std::string::npos)
	{
		return Fail("Effect loader accepted an ID without a variant segment") ? 0 : 1;
	}
	std::string effectCatalogFailure;
	if (!ly::content::EffectContentCatalog::LoadFromFile(
			effectPath,
			EffectData::GetBuiltinGameplayEffectDefinitions(),
			&effectCatalogFailure
		) ||
		EffectData::FindGameplayEffectDefinition("Effect.Barrier.Basic") == nullptr ||
		!EffectData::FindGameplayEffectDefinition("Effect.Barrier.Basic")->sourceParameterized ||
		!NearlyEqual(EffectData::FindGameplayEffectDefinition("Effect.Barrier.Basic")->duration, 0.f))
	{
		return Fail(effectCatalogFailure.empty()
			? "Shield effect catalog did not expose the JSON definition"
			: effectCatalogFailure.c_str()) ? 0 : 1;
	}
	const std::filesystem::path partialEffectPath =
		std::filesystem::temp_directory_path() / "lightyears_partial_effect_catalog.json";
	{
		std::ofstream partialEffectFile{ partialEffectPath };
		partialEffectFile << R"({
  "schemaVersion": 1,
  "effects": [{
    "id": "Effect.Barrier.Basic",
    "durationPolicy": "Duration",
    "stackingPolicy": "RefreshDuration",
    "sourceParameterized": true
  }]
})";
	}
	std::string partialEffectCatalogFailure;
	const bool partialEffectCatalogLoaded =
		ly::content::EffectContentCatalog::LoadFromFile(
			partialEffectPath,
			EffectData::GetBuiltinGameplayEffectDefinitions(),
			&partialEffectCatalogFailure
		);
	std::filesystem::remove(partialEffectPath);
	if (!partialEffectCatalogLoaded ||
		ly::content::EffectContentCatalog::FindById("Effect.Barrier.Basic") == nullptr ||
		ly::content::EffectContentCatalog::FindById("Effect.Status.Damage.Ignite") != nullptr)
	{
		return Fail("Effect catalog reintroduced a C++ fallback record missing from JSON") ? 0 : 1;
	}

	std::cout << "Ability, weapon, ship, attachment, effect, and enemy combat profile content loader tests passed\n";
	return 0;
}
