#include "gameplay/content/AttachmentLoader.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/EffectLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/content/WeaponLoader.h"
#include "gameplay/content/AbilityLoader.h"

#include "attributes/GameplayAttribute.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/functional/DashConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/combat/EffectConfig.h"

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
	bool NearlyEqual(float left, float right, float tolerance = 0.0001f)
	{
		return std::fabs(left - right) <= tolerance;
	}

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

int main()
{
	const std::filesystem::path abilityPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/abilities.json";
	const ly::List<const ly::GameAbilityDefinition*> fallbackAbilities{
		&AbilityData::Definitions::Dash_Basic,
		&AbilityData::Definitions::Shield_Basic,
		&AbilityData::Definitions::SunBeam_Strike_Basic,
		&AbilityData::Definitions::Rocket_Basic,
		&AbilityData::Definitions::GravityAnomaly_Basic,
		&AbilityData::Definitions::InfernoSpray_Basic
	};
	const ly::List<const ly::AbilityActorDefinition*> fallbackAbilityActors{
		&AbilityData::GravityAnomaly::ActorProjectileBasic,
		&AbilityData::GravityAnomaly::ActorFieldBasic,
		&AbilityData::Rocket::ActorProjectileBasic,
		&AbilityData::InfernoSpray::ActorFlameConeBasic,
		&AbilityData::SunBeam::ActorStrikeBasic
	};
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
	if (loadedAbilities.definitions.size() != 6)
	{
		return Fail("Ability JSON catalog did not load the expected pilot definitions") ? 0 : 1;
	}
	std::size_t actorDefinitionCount = 0;
	for (const auto& definition : loadedAbilities.definitions)
	{
		actorDefinitionCount += definition.actorDefinitions.size();
	}
	if (actorDefinitionCount != 5)
	{
		return Fail("Ability actor JSON catalog did not load the expected actor definitions") ? 0 : 1;
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
      "id": "Ability.Dash.Basic",
      "cooldown": 2.0,
      "duration": 0.24,
      "maxCharges": 1,
      "settings": { "baseDistance": 260.0, "cameraZoomOutRatio": 0.15 }
    },
    {
      "id": "Ability.Dash.Basic",
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

	const std::filesystem::path abilityVariantPath =
		std::filesystem::temp_directory_path() /
		"lightyears_ability_variant_test.json";
	{
		std::ofstream variantFile{ abilityVariantPath };
		variantFile << R"({
  "schemaVersion": 1,
  "abilities": [
    {
      "id": "Ability.Dash.Basic",
      "cooldown": 2.0,
      "duration": 0.24,
      "maxCharges": 1,
      "settings": { "baseDistance": 333.0, "cameraZoomOutRatio": 0.15 }
    },
    {
      "id": "Ability.Dash.Variant",
      "baseId": "Ability.Dash.Basic",
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
			"Ability.Dash.Variant" ||
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
	const std::filesystem::path missingAbilityNumbersPath =
		std::filesystem::temp_directory_path() /
		"lightyears_missing_ability_numbers_test.json";
	{
		std::ofstream missingAbilityNumbersFile{ missingAbilityNumbersPath };
		missingAbilityNumbersFile << R"({
  "schemaVersion": 1,
  "abilities": [{
    "id": "Ability.Dash.Basic",
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
	if (dash.id != "Ability.Dash.Basic" ||
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
			return definition.id == "Ability.Shield.Basic";
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
			sas::FindGameplayAttributeValue(
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
			return definition.id == "Ability.SunBeam.Strike.Basic";
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
			return definition.id == "Ability.Rocket.Basic";
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
			return definition.id == "Ability.GravityAnomaly.Basic";
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
			sas::FindGameplayAttributeValue(
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
			return definition.id == "Ability.InfernoSpray.Basic";
		}
	);
	if (infernoIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(infernoIt->definition.cooldown, 3.f) ||
		!NearlyEqual(infernoIt->definition.duration, 3.f) ||
		infernoIt->definition.maxCharges != 1)
	{
		return Fail("Inferno Spray ability JSON profile is invalid") ? 0 : 1;
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
	if (loaded.definitions.size() != 6)
	{
		return Fail("Weapon JSON catalog did not load exactly six definitions") ? 0 : 1;
	}

	const char* expectedWeaponIds[] = {
		"FighterBasicRapidLaser",
		"PlayerRapidShotgun",
		"PlayerDualKineticBlaster",
		"PlayerElectricArcLauncher",
		"PlayerContinuousHeatLaser",
		"PlayerCryoWaveProjector"
	};
	for (const char* expectedId : expectedWeaponIds)
	{
		if (!FindWeapon(loaded.definitions, expectedId))
		{
			return Fail("Weapon JSON catalog is missing an expected weapon") ? 0 : 1;
		}
	}

	const PrimaryWeaponDefinition* basicLaser =
		FindWeapon(loaded.definitions, "FighterBasicRapidLaser");
	const sas::GameplayAttribute* basicDamage = sas::FindGameplayAttribute(
		basicLaser->attributes,
		ly::CommonAttributeIds::Damage
	);
	if (basicLaser->weaponTypeTag != PrimaryWeaponSchema::Projectile::Standard::TypeId ||
		!basicDamage ||
		!NearlyEqual(basicDamage->baseValue, 8.f) ||
		basicLaser->progressionProfile.ResolveLevelSteps().size() != 3 ||
		basicLaser->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Photonic })
	{
		return Fail("Basic rapid laser JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* shotgun =
		FindWeapon(loaded.definitions, "PlayerRapidShotgun");
	if (shotgun->weaponTypeTag != PrimaryWeaponSchema::Projectile::Shotgun::TypeId ||
		shotgun->muzzleDefinitions.size() != 1 ||
		shotgun->attributes.size() != 16 ||
		!sas::FindGameplayAttribute(shotgun->attributes, ly::DamageAttributeIds::BurnDuration) ||
		shotgun->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Thermal })
	{
		return Fail("Rapid shotgun JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* dual =
		FindWeapon(loaded.definitions, "PlayerDualKineticBlaster");
	if (dual->muzzleDefinitions.size() != 2 ||
		dual->progressionProfile.ResolveLevelSteps().size() != 3 ||
		dual->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Kinetic })
	{
		return Fail("Dual kinetic blaster JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* electric =
		FindWeapon(loaded.definitions, "PlayerElectricArcLauncher");
	if (electric->weaponTypeTag != PrimaryWeaponSchema::Arc::Electric::TypeId ||
		!sas::FindGameplayAttribute(
			electric->attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainCount
		) ||
		electric->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Electric })
	{
		return Fail("Electric arc launcher JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* beam =
		FindWeapon(loaded.definitions, "PlayerContinuousHeatLaser");
	if (beam->weaponTypeTag != PrimaryWeaponSchema::Beam::Continuous::TypeId ||
		beam->featureTags != ly::List<ly::GameplayTag>{ PrimaryWeaponSchema::Feature::Heat::FeatureId } ||
		beam->heatGainCurve.size() != 4 ||
		beam->damageTags != ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Energy })
	{
		return Fail("Continuous heat laser JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* cryo =
		FindWeapon(loaded.definitions, "PlayerCryoWaveProjector");
	if (cryo->weaponTypeTag != PrimaryWeaponSchema::Wave::Expanding::TypeId ||
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
	if (!loadedShips.Succeeded() || loadedShips.definitions.size() != 1 ||
		loadedShips.definitions.front().id != "Player.Fighter" ||
		loadedShips.definitions.front().definition.primaryWeaponId !=
			"FighterBasicRapidLaser" ||
		!NearlyEqual(loadedShips.definitions.front().definition.health, 100.f))
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
			ly::GameplayTag{ "Attachment.Thermal.Converter" } ||
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
	if (!loadedEffects.Succeeded() || loadedEffects.definitions.size() != 8 ||
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

	std::cout << "Ability, weapon, ship, attachment, and effect content loader tests passed\n";
	return 0;
}
