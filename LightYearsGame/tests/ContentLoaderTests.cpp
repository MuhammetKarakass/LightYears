#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/EffectLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameConfigs/ship/ShipConfig.h"
#include "framework/JsonDocumentLoader.h"
#include "gameplay/content/WeaponLoader.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/AbilityLoader.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/EnemyCombatProfileLoader.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
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
#include "gameConfigs/ability/utility/GravityAnomalyConfig.h"
#include "gameConfigs/ability/utility/FrostMaelstromConfig.h"
#include "gameConfigs/ability/offensive/FrozenThrongConfig.h"
#include "gameConfigs/ability/offensive/CombatSentryConfig.h"
#include "gameConfigs/ability/offensive/NanoPlagueConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/OverdriveCoreConfig.h"
#include "gameConfigs/ability/offensive/OrbitalDronesConfig.h"
#include "gameConfigs/ability/offensive/ExecutionDriveConfig.h"
#include "gameConfigs/ability/utility/NullPulseConfig.h"
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
#include "gameConfigs/ability/movement/ScorchDriveConfig.h"
#include "gameConfigs/ability/offensive/IonStormConfig.h"
#include "gameConfigs/ability/offensive/ChainLightningConfig.h"
#include "gameConfigs/ability/offensive/GlacialPressureConfig.h"
#include "gameConfigs/ability/offensive/MineLayerConfig.h"
#include "gameConfigs/ability/movement/EnergySpearConfig.h"
#include "gameConfigs/combat/EffectConfig.h"

#include <cmath>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <variant>

namespace vega
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
		const vega::List<PrimaryWeaponDefinition>& definitions,
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

	struct ExpectedAttributeValue
	{
		sas::AttributeId attributeId;
		float value;
	};

	struct ExpectedPlayerShip
	{
		struct OwnerAttributes
		{
			float attackPower;
			float armor;
			float luck;
			float attackSpeed;
		};

		struct Growth
		{
			float maxHealth;
			float attackPower;
			float energyPower;
			float armor;
			float luck;
			float attackSpeed;
		};

		struct Movement
		{
			float maxSpeed;
			float forwardThrust;
			float reverseThrust;
			float strafeThrust;
			float angularTurnSpeed;
			float angularTurnResponsiveness;
		};

		struct Energy
		{
			float energyPower;
			float maxShield;
			float maxShieldPerLevel;
			float shieldRechargeDuration;
			float shieldRechargeDelay;
			float afterburnerCapacity;
			float afterburnerCapacityPerLevel;
			float afterburnerRechargeDuration;
			float afterburnerRechargeDelay;
			float afterburnerSpeedMultiplier;
			float afterburnerAccelerationMultiplier;
			float afterburnerManeuverabilityMultiplier;
			float afterburnerEnergyDrainPerSecond;
			float afterburnerRampUpDuration;
			float afterburnerRampDownDuration;
		};

		const char* id;
		const char* texturePath;
		const char* primaryWeaponId;
		float health;
		OwnerAttributes ownerAttributes;
		Growth growth;
		Movement movement;
		Energy energy;
	};

	int ValidateShippedPlayerRoster(
		const std::filesystem::path& shipPath,
		const vega::content::ShipLoader::Result& loadedShips
	)
	{
		const std::array<ExpectedPlayerShip, 6> expectedShips{{
			{
				"Ship.Player.Fighter.Basic", "SpaceShooterRedux/PNG/playerShip1_blue.png",
				"Weapon.Projectile.FighterRapidLaser.Basic", 250.f, { 39.f, 2.f, 0.f, 0.f },
				{ 40.f, 3.f, 2.f, 2.f, 1.f, 0.f }, { 520.f, 650.f, 190.f, 270.f, 400.f, 5.f },
				{ 35.f, 140.f, 20.f, 6.f, 6.f, 100.f, 3.f, 6.f, 1.5f, 1.55f, 1.8f, 0.9f, 33.f, 0.22f, 0.45f }
			},
			{
				"Ship.Player.Breacher.Basic", "SpaceShooterRedux/PNG/playerShip2_orange.png",
				"Weapon.Cone.RapidShotgun.Basic", 290.f, { 45.f, 3.f, 0.f, 0.f },
				{ 60.f, 2.f, 0.f, 2.f, 1.f, 0.f }, { 490.f, 750.f, 220.f, 245.f, 450.f, 6.f },
				{ 34.f, 110.f, 10.f, 6.f, 6.f, 110.f, 2.f, 5.f, 1.2f, 1.65f, 2.f, 0.9f, 33.f, 0.22f, 0.45f }
			},
			{
				"Ship.Player.Interceptor.Basic", "SpaceShooterRedux/PNG/playerShip3_red.png",
				"Weapon.Projectile.DualKineticBlaster.Basic", 200.f, { 41.f, 1.f, 1.f, 1.f },
				{ 30.f, 3.f, 1.f, 1.f, 1.f, 1.f }, { 590.f, 720.f, 240.f, 340.f, 480.f, 6.5f },
				{ 28.f, 110.f, 15.f, 6.f, 6.f, 120.f, 4.f, 5.f, 1.f, 1.6f, 2.f, 1.f, 30.f, 0.22f, 0.45f }
			},
			{
				"Ship.Player.Conductor.Basic", "SpaceShooterRedux/PNG/playerShip3_blue.png",
				"Weapon.Arc.ElectricLauncher.Basic", 220.f, { 27.f, 1.f, 2.f, 0.f },
				{ 40.f, 1.f, 3.f, 1.f, 2.f, 0.f }, { 505.f, 620.f, 200.f, 285.f, 410.f, 5.3f },
				{ 41.f, 130.f, 20.f, 5.f, 5.f, 95.f, 3.f, 6.f, 1.5f, 1.5f, 1.8f, 0.9f, 33.f, 0.22f, 0.45f }
			},
			{
				"Ship.Player.Aegis.Basic", "SpaceShooterRedux/PNG/playerShip2_green.png",
				"Weapon.Beam.ContinuousHeatLaser.Basic", 270.f, { 25.f, 4.f, 0.f, 0.f },
				{ 50.f, 1.f, 3.f, 3.f, 1.f, 0.f }, { 480.f, 600.f, 200.f, 240.f, 380.f, 4.8f },
				{ 43.f, 160.f, 30.f, 7.f, 4.5f, 75.f, 2.f, 7.f, 2.f, 1.45f, 1.6f, 0.85f, 36.f, 0.22f, 0.45f }
			},
			{
				"Ship.Player.CryoController.Basic", "SpaceShooterRedux/PNG/playerShip2_blue.png",
				"Weapon.Wave.CryoProjector.Basic", 250.f, { 27.f, 2.f, 1.f, 0.f },
				{ 40.f, 1.f, 2.f, 2.f, 1.f, 0.f }, { 520.f, 610.f, 210.f, 310.f, 425.f, 5.5f },
				{ 40.f, 130.f, 20.f, 6.f, 5.5f, 95.f, 3.f, 6.f, 1.5f, 1.5f, 1.8f, 0.9f, 33.f, 0.22f, 0.45f }
			}
		}};
		const std::array<const char*, 4> expectedEnemyIds{{
			"Ship.Enemy.ApproachGunner.Basic",
			"Ship.Enemy.StrafeSkirmisher.Basic",
			"Ship.Enemy.RangeKeeper.Basic",
			"Ship.Enemy.HeavyDummy.Basic"
		}};
		if (!loadedShips.Succeeded() || loadedShips.definitions.size() != 10)
		{
			return Fail("Ship JSON must contain six player ships and four enemy ships") ? 0 : 1;
		}

		vega::List<std::string> expectedPlayerShipIds;
		for (const ExpectedPlayerShip& expected : expectedShips)
		{
			expectedPlayerShipIds.push_back(expected.id);
		}
		if (vega::content::ShipContentCatalog::GetPlayerShipIds() != expectedPlayerShipIds)
		{
			return Fail("Player ship catalog IDs do not match the six-ship roster order") ? 0 : 1;
		}

		const auto entriesMatch = [](
			const auto& actualEntries,
			const auto& expectedEntries,
			const auto& getActualValue
		)
		{
			const std::size_t expectedCount = static_cast<std::size_t>(std::count_if(
				expectedEntries.begin(), expectedEntries.end(),
				[](const ExpectedAttributeValue& entry) { return entry.value > 0.f; }
			));
			if (actualEntries.size() != expectedCount)
			{
				return false;
			}
			for (std::size_t index = 0; index < actualEntries.size(); ++index)
			{
				const auto& actual = actualEntries[index];
				const auto expected = std::find_if(
					expectedEntries.begin(), expectedEntries.end(),
					[&](const ExpectedAttributeValue& entry) { return entry.attributeId == actual.attributeId; }
				);
				if (expected == expectedEntries.end() || expected->value <= 0.f ||
					!NearlyEqual(getActualValue(actual), expected->value))
				{
					return false;
				}
				for (std::size_t previous = 0; previous < index; ++previous)
				{
					if (actualEntries[previous].attributeId == actual.attributeId)
					{
						return false;
					}
				}
			}
			return true;
		};

		for (std::size_t index = 0; index < expectedShips.size(); ++index)
		{
			const ExpectedPlayerShip& expected = expectedShips[index];
			const vega::content::ShipLoader::LoadedDefinition& loaded = loadedShips.definitions[index];
			const ShipDefinition& ship = loaded.definition;
			if (loaded.id != expected.id || ship.texturePath != expected.texturePath ||
				ship.primaryWeaponId != expected.primaryWeaponId ||
				!vega::content::WeaponContentCatalog::FindById(expected.primaryWeaponId) ||
				!NearlyEqual(ship.health, expected.health) ||
				!NearlyEqual(ship.speed.x, 350.f) || !NearlyEqual(ship.speed.y, 350.f) ||
				!NearlyEqual(ship.collisionDamage, 25.f) || ship.scoreAmt != 0u ||
				!NearlyEqual(ship.shipXPReward, 0.f) ||
				!NearlyEqual(ship.progressionDefinition.baseXP, 6.f) ||
				!NearlyEqual(ship.progressionDefinition.xpExponent, 0.f) ||
				!NearlyEqual(ship.progressionDefinition.xpPerLevel, 1.f))
			{
				return Fail("Player ship identity, presentation, weapon, or shared stats are invalid") ? 0 : 1;
			}

			const ExpectedPlayerShip::Movement& movement = expected.movement;
			const ShipMovementAttributes& actualMovement = ship.movementAttributes;
			if (!NearlyEqual(actualMovement.maxSpeed.currentValue, movement.maxSpeed) ||
				!NearlyEqual(actualMovement.forwardThrust.currentValue, movement.forwardThrust) ||
				!NearlyEqual(actualMovement.reverseThrust.currentValue, movement.reverseThrust) ||
				!NearlyEqual(actualMovement.strafeThrust.currentValue, movement.strafeThrust) ||
				!NearlyEqual(actualMovement.angularTurnSpeed.currentValue, movement.angularTurnSpeed) ||
				!NearlyEqual(actualMovement.angularTurnResponsiveness.currentValue, movement.angularTurnResponsiveness) ||
				!NearlyEqual(actualMovement.linearDamping.currentValue, 0.70f) ||
				!NearlyEqual(actualMovement.inputResponsiveness.currentValue, 12.f) ||
				!NearlyEqual(actualMovement.mouseAimDeadZone.currentValue, 32.f) ||
				!NearlyEqual(actualMovement.horizontalRatingToThrust, 60.f) ||
				!NearlyEqual(actualMovement.verticalRatingToThrust, 60.f) ||
				!NearlyEqual(actualMovement.ratingToMaxSpeed, 20.f) ||
				!NearlyEqual(actualMovement.movementRatingDiminishingScale, 20.f))
			{
				return Fail("Player ship movement does not match its approved profile") ? 0 : 1;
			}

			const ExpectedPlayerShip::Energy& energy = expected.energy;
			const ShipEnergyAttributes& actualEnergy = ship.energyAttributes;
			if (!NearlyEqual(actualEnergy.baseEnergyPower, energy.energyPower) ||
				!NearlyEqual(actualEnergy.baseMaxShield, energy.maxShield) ||
				!NearlyEqual(actualEnergy.maxShieldPerLevel, energy.maxShieldPerLevel) ||
				!NearlyEqual(actualEnergy.shieldFullRechargeDuration, energy.shieldRechargeDuration) ||
				!NearlyEqual(actualEnergy.baseShieldRechargeDelay, energy.shieldRechargeDelay) ||
				!NearlyEqual(actualEnergy.baseAfterburnerCapacity, energy.afterburnerCapacity) ||
				!NearlyEqual(actualEnergy.afterburnerCapacityPerLevel, energy.afterburnerCapacityPerLevel) ||
				!NearlyEqual(actualEnergy.afterburnerFullRechargeDuration, energy.afterburnerRechargeDuration) ||
				!NearlyEqual(actualEnergy.baseAfterburnerRechargeDelay, energy.afterburnerRechargeDelay) ||
				!NearlyEqual(actualEnergy.baseAfterburnerSpeedMultiplier, energy.afterburnerSpeedMultiplier) ||
				!NearlyEqual(actualEnergy.baseAfterburnerAccelerationMultiplier, energy.afterburnerAccelerationMultiplier) ||
				!NearlyEqual(actualEnergy.baseAfterburnerManeuverabilityMultiplier, energy.afterburnerManeuverabilityMultiplier) ||
				!NearlyEqual(actualEnergy.baseAfterburnerEnergyDrainPerSecond, energy.afterburnerEnergyDrainPerSecond) ||
				!NearlyEqual(actualEnergy.baseAfterburnerRampUpDuration, energy.afterburnerRampUpDuration) ||
				!NearlyEqual(actualEnergy.baseAfterburnerRampDownDuration, energy.afterburnerRampDownDuration))
			{
				return Fail("Player ship shield or afterburner profile is invalid") ? 0 : 1;
			}

			const std::array<ExpectedAttributeValue, 4> expectedOwnerAttributes{{
				{ vega::OwnerAttributeIds::AttackPower, expected.ownerAttributes.attackPower },
				{ vega::OwnerAttributeIds::Armor, expected.ownerAttributes.armor },
				{ vega::OwnerAttributeIds::Luck, expected.ownerAttributes.luck },
				{ vega::OwnerAttributeIds::AttackSpeed, expected.ownerAttributes.attackSpeed }
			}};
			const std::array<ExpectedAttributeValue, 6> expectedGrowth{{
				{ vega::OwnerAttributeIds::MaxHealth, expected.growth.maxHealth },
				{ vega::OwnerAttributeIds::AttackPower, expected.growth.attackPower },
				{ vega::OwnerAttributeIds::EnergyPower, expected.growth.energyPower },
				{ vega::OwnerAttributeIds::Armor, expected.growth.armor },
				{ vega::OwnerAttributeIds::Luck, expected.growth.luck },
				{ vega::OwnerAttributeIds::AttackSpeed, expected.growth.attackSpeed }
			}};
			if (!entriesMatch(ship.baseOwnerAttributes, expectedOwnerAttributes,
				[](const OwnerAttributeBaseEntry& entry) { return entry.baseValue; }) ||
				!entriesMatch(ship.progressionDefinition.naturalGrowth, expectedGrowth,
					[](const AttributeGrowthEntry& entry) { return entry.perLevel; }))
			{
				return Fail("Player ship base attributes or natural growth are not minimal and exact") ? 0 : 1;
			}
		}

		for (std::size_t index = 0; index < expectedEnemyIds.size(); ++index)
		{
			if (loadedShips.definitions[expectedShips.size() + index].id != expectedEnemyIds[index])
			{
				return Fail("The four enemy ship definitions must remain after the six player ships") ? 0 : 1;
			}
		}

		const vega::JsonDocumentLoader::Result shipDocument =
			vega::JsonDocumentLoader::LoadFromFile(shipPath);
		if (!shipDocument.Succeeded())
		{
			return Fail("Ship JSON could not be parsed for removed-affinity checks") ? 0 : 1;
		}
		for (const auto& ship : shipDocument.document->at("ships"))
		{
			const auto& energy = ship.at("energy");
			if (energy.contains("shieldAffinity") || energy.contains("afterburnerAffinity"))
			{
				return Fail("Ship JSON must not contain EP-based defense affinity fields") ? 0 : 1;
			}
		}

		// Engine light offsets: the five new hulls place theirs from JSON; Fighter
		// keeps the presentation base's mounts (this fixture's base has none).
		const auto mountsAt = [](const vega::List<EngineMount>& mounts, float x, float y)
		{
			return mounts.size() == 2 &&
				NearlyEqual(mounts[0].offset.x, -x) && NearlyEqual(mounts[0].offset.y, y) &&
				NearlyEqual(mounts[1].offset.x, x) && NearlyEqual(mounts[1].offset.y, y);
		};
		for (const auto& loaded : loadedShips.definitions)
		{
			if (loaded.id.rfind("Ship.Player.", 0) != 0 || loaded.id == "Ship.Player.Fighter.Basic") continue;
			if (!mountsAt(loaded.definition.engineMounts, 29.f, -16.f))
			{
				return Fail(("Player ship engine mounts are wrong for " + loaded.id).c_str()) ? 0 : 1;
			}
		}
		if (!mountsAt(ShipData::Ship_Player_Fighter.engineMounts, 22.f, -12.f))
		{
			return Fail("Fighter presentation base engine mounts changed") ? 0 : 1;
		}

		{
			const std::filesystem::path badMountsPath =
				std::filesystem::temp_directory_path() / "lightyears-bad-engine-mounts.json";
			std::string badShips = shipDocument.document->dump();
			const std::string fighterId = "\"id\":\"Ship.Player.Fighter.Basic\"";
			const std::size_t fighterAt = badShips.find(fighterId);
			if (fighterAt == std::string::npos)
			{
				return Fail("Engine mount fixture could not find the Fighter entry") ? 0 : 1;
			}
			badShips.insert(fighterAt + fighterId.size(), ",\"engineMounts\":5");
			{
				std::ofstream file{ badMountsPath, std::ios::trunc };
				file << badShips;
			}
			const vega::content::ShipLoader::Result badResult =
				vega::content::ShipLoader::LoadFromFile(badMountsPath, ShipData::Ship_Player_Fighter);
			std::error_code removeError;
			std::filesystem::remove(badMountsPath, removeError);
			if (badResult.Succeeded() || badResult.error.find("engineMounts must be an array") == std::string::npos)
			{
				return Fail("Ship loader accepted a non-array engineMounts value") ? 0 : 1;
			}
		}

		return 0;
	}
}

namespace
{
	int RunDamageStatusBalanceContentTests()
	{
		const std::filesystem::path balancePath =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data/damage_status_balance.json";
		std::string failureReason;
		if (!vega::content::DamageStatusBalanceCatalog::LoadFromFile(
			balancePath,
			&failureReason
		) || !vega::content::DamageStatusBalanceCatalog::IsLoaded())
		{
			return Fail("Damage status balance JSON did not load") ? 0 : 1;
		}

		const vega::DamageStatusBalance& balance =
			vega::content::DamageStatusBalanceCatalog::Get();
		if (!NearlyEqual(balance.energyShieldDamageMultiplier, 1.50f) ||
			balance.cryo.maxStacks != 4 ||
			!NearlyEqual(balance.cryo.duration, 6.f) ||
			!NearlyEqual(balance.electric.duration, 6.f) ||
			!NearlyEqual(balance.cryo.values[3], 0.20f) ||
			balance.electric.maxStacks != 4 ||
			!NearlyEqual(balance.electric.values[0], 0.03f) ||
			!NearlyEqual(balance.ThermalDamagePerSecond(4), 5.f) ||
			!NearlyEqual(balance.KineticArmorShred(4), 0.30f))
		{
			return Fail("Damage status balance acceptance values are invalid") ? 0 : 1;
		}

		const std::filesystem::path invalidPath =
			std::filesystem::temp_directory_path() /
			"lightyears_invalid_damage_status_balance.json";
		{
			std::ofstream invalidFile{ invalidPath };
			invalidFile << R"({
  "schemaVersion": 1,
  "energyShieldDamageMultiplier": 1.5,
  "statuses": {}
})";
		}
		failureReason.clear();
		const bool invalidLoaded =
			vega::content::DamageStatusBalanceCatalog::LoadFromFile(
				invalidPath,
				&failureReason
			);
		std::filesystem::remove(invalidPath);
		bool invalidGetRejected = false;
		try
		{
			vega::content::DamageStatusBalanceCatalog::Get();
		}
		catch (const std::logic_error&)
		{
			invalidGetRejected = true;
		}
		if (invalidLoaded || failureReason.empty() ||
			vega::content::DamageStatusBalanceCatalog::IsLoaded() || !invalidGetRejected)
		{
			return Fail("Invalid damage status balance JSON did not fail closed") ? 0 : 1;
		}

		const std::filesystem::path missingPath =
			std::filesystem::temp_directory_path() /
			"lightyears_missing_damage_status_balance.json";
		std::filesystem::remove(missingPath);
		failureReason.clear();
		const bool missingLoaded =
			vega::content::DamageStatusBalanceCatalog::LoadFromFile(
				missingPath,
				&failureReason
			);
		bool missingGetRejected = false;
		try
		{
			vega::content::DamageStatusBalanceCatalog::Get();
		}
		catch (const std::logic_error&)
		{
			missingGetRejected = true;
		}
		if (missingLoaded || failureReason.empty() ||
			vega::content::DamageStatusBalanceCatalog::IsLoaded() || !missingGetRejected)
		{
			return Fail("Missing damage status balance JSON did not fail closed") ? 0 : 1;
		}

		failureReason.clear();
		if (!vega::content::DamageStatusBalanceCatalog::LoadFromFile(
			balancePath,
			&failureReason
		) || !vega::content::DamageStatusBalanceCatalog::IsLoaded() ||
			!NearlyEqual(
				vega::content::DamageStatusBalanceCatalog::Get().energyShieldDamageMultiplier,
				1.50f
			))
		{
			return Fail("Damage status balance catalog did not recover after a failed reload") ? 0 : 1;
		}

		std::cout << "[PASS] Damage status balance content tests passed successfully!\n";
		return 0;
	}

	int RunFailedReloadContentTests()
	{
		const std::filesystem::path dataDirectory =
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
			"LightYearsGame/assets/content/data";
		const std::filesystem::path abilityPath = dataDirectory / "abilities.json";
		const std::filesystem::path effectPath = dataDirectory / "effects.json";
		const std::filesystem::path weaponPath = dataDirectory / "weapons.json";
		const std::filesystem::path shipPath = dataDirectory / "ships.json";
		const std::filesystem::path invalidPath =
			std::filesystem::temp_directory_path() / "lightyears_invalid_catalog_reload.json";
		const std::filesystem::path missingPath =
			std::filesystem::temp_directory_path() / "lightyears_missing_catalog_reload.json";

		const auto exerciseReload = [&](const char* catalogName,
			const std::filesystem::path& validPath,
			auto load,
			auto isLoaded,
			auto hidesStaleRecords,
			auto exposesRecoveredRecords)
		{
			std::string failureReason;
			if (!load(validPath, &failureReason) || !isLoaded() || !exposesRecoveredRecords())
			{
				return Fail((std::string{ catalogName } + " catalog failed its initial load")
					.c_str());
			}

			for (const std::filesystem::path& failedPath : { invalidPath, missingPath })
			{
				if (failedPath == invalidPath)
				{
					std::ofstream invalidFile{ invalidPath };
					invalidFile << "{ invalid JSON";
				}
				else
				{
					std::filesystem::remove(missingPath);
				}

				failureReason.clear();
				const bool reloaded = load(failedPath, &failureReason);
				if (failedPath == invalidPath)
				{
					std::filesystem::remove(invalidPath);
				}
				if (reloaded || failureReason.empty() || isLoaded() || !hidesStaleRecords())
				{
					return Fail((std::string{ catalogName } +
						" catalog exposed stale data after a failed reload")
						.c_str());
				}
				failureReason.clear();
				if (!load(validPath, &failureReason) || !isLoaded() ||
					!exposesRecoveredRecords())
				{
					return Fail((std::string{ catalogName } +
						" catalog failed to recover after a failed reload")
						.c_str());
				}
			}
			return true;
		};

		const std::string abilityId = "Ability.Offense.AstralSurge.Basic";
		if (!exerciseReload(
			"Ability",
			abilityPath,
			[](const std::filesystem::path& path, std::string* error)
			{
				return vega::content::AbilityContentCatalog::LoadFromFile(
					path,
					AbilityData::GetBuiltinShippedAbilityDefinitions(),
					AbilityData::GetBuiltinAbilityActorDefinitions(),
					error
				);
			},
			[] { return vega::content::AbilityContentCatalog::IsLoaded(); },
			[&]
			{
				return vega::content::AbilityContentCatalog::FindById(abilityId) == nullptr &&
					vega::content::AbilityContentCatalog::GetDefinitions().empty();
			},
			[&] { return vega::content::AbilityContentCatalog::FindById(abilityId) != nullptr; }
		)) return 1;

		const std::string effectId = "Effect.Barrier.Basic";
		if (!exerciseReload(
			"Effect",
			effectPath,
			[](const std::filesystem::path& path, std::string* error)
			{
				return vega::content::EffectContentCatalog::LoadFromFile(
					path,
					EffectData::GetBuiltinGameplayEffectDefinitions(),
					error
				);
			},
			[] { return vega::content::EffectContentCatalog::IsLoaded(); },
			[&]
			{
				return vega::content::EffectContentCatalog::FindById(effectId) == nullptr &&
					vega::content::EffectContentCatalog::GetDefinitions().empty();
			},
			[&] { return vega::content::EffectContentCatalog::FindById(effectId) != nullptr; }
		)) return 1;

		const std::string weaponId = "Weapon.Projectile.FighterRapidLaser.Basic";
		if (!exerciseReload(
			"Weapon",
			weaponPath,
			[](const std::filesystem::path& path, std::string* error)
			{
				return vega::content::WeaponContentCatalog::LoadFromFile(path, error);
			},
			[] { return vega::content::WeaponContentCatalog::IsLoaded(); },
			[&]
			{
				return vega::content::WeaponContentCatalog::FindById(weaponId) == nullptr &&
					!vega::content::WeaponContentCatalog::ResolveAuthoredAttributeAtLevel(
						weaponId, 1, vega::CommonAttributeIds::Damage).has_value();
			},
			[&] { return vega::content::WeaponContentCatalog::FindById(weaponId) != nullptr; }
		)) return 1;

		const std::string shipId = "Ship.Player.Fighter.Basic";
		if (!exerciseReload(
			"Ship",
			shipPath,
			[](const std::filesystem::path& path, std::string* error)
			{
				return vega::content::ShipContentCatalog::LoadFromFile(path, error);
			},
			[] { return vega::content::ShipContentCatalog::IsLoaded(); },
			[&] { return vega::content::ShipContentCatalog::FindById(shipId) == nullptr; },
			[&] { return vega::content::ShipContentCatalog::FindById(shipId) != nullptr; }
		)) return 1;

		std::cout << "[PASS] Failed catalog reload content tests passed successfully\n";
		return 0;
	}

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

		if (!vega::content::WeaponContentCatalog::IsLoaded())
		{
			vega::content::WeaponContentCatalog::LoadFromFile(weaponPath);
		}
		if (!vega::content::AbilityContentCatalog::IsLoaded())
		{
			std::string abilityLoadErr;
			if (!vega::content::AbilityContentCatalog::LoadFromFile(
				abilitiesPath,
				AbilityData::GetBuiltinShippedAbilityDefinitions(),
				AbilityData::GetBuiltinAbilityActorDefinitions(),
				&abilityLoadErr))
			{
				return Fail(("Failed to load AbilityContentCatalog: " + abilityLoadErr).c_str()) ? 0 : 1;
			}
		}

		const vega::content::EnemyCombatProfileLoader::Result loadedEnemyProfiles =
			vega::content::EnemyCombatProfileLoader::LoadFromFile(enemyProfilePath);
		if (!loadedEnemyProfiles.Succeeded())
		{
			return Fail(loadedEnemyProfiles.error.c_str()) ? 0 : 1;
		}
		if (loadedEnemyProfiles.profiles.size() != 3)
		{
			return Fail("Enemy combat profiles did not load exactly three profiles") ? 0 : 1;
		}

		std::string enemyCatalogError;
		if (!vega::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath, &enemyCatalogError))
		{
			return Fail(enemyCatalogError.c_str()) ? 0 : 1;
		}

	const vega::EnemyCombatProfile* vanguardProfile =
			vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
		const vega::EnemyCombatProfile* twinBladeProfile =
			vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.StrafeSkirmisher.Basic");
		const vega::EnemyCombatProfile* hexagonProfile =
			vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.RangeKeeper.Basic");

		if (!vanguardProfile || !twinBladeProfile || !hexagonProfile)
		{
			return Fail("Catalog failed to find one of the three basic enemy profiles") ? 0 : 1;
		}

		if (vanguardProfile->weapons.size() != 1 || vanguardProfile->weapons.front().weaponId != "Weapon.Projectile.EnemyVanguardPulse.Basic" ||
			vanguardProfile->powerScalingPolicy != vega::EnemyPowerScalingPolicy::Disabled)
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
		const vega::content::EnemyCombatProfileLoader::Result dupLoadResult =
			vega::content::EnemyCombatProfileLoader::LoadFromFile(duplicateProfilePath);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(missingWeaponPath, &missingWeaponError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(missingAbilityPath, &missingAbilityError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(dupSlotPath, &dupSlotError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(disabledScalingPath, &disabledScalingError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(allowedScalingPath, &allowedScalingError);
		std::filesystem::remove(allowedScalingPath);
		if (!allowedScalingLoaded)
		{
			return Fail(allowedScalingError.c_str()) ? 0 : 1;
		}
		const vega::EnemyCombatProfile* allowedProfile =
			vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.Test.Basic");
		if (!allowedProfile || allowedProfile->powerScalingPolicy != vega::EnemyPowerScalingPolicy::Allowed)
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(dupAbilityIdPath, &dupAbilityIdError);
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
		vega::EnemyCombatProfile invalidEnemyGrowth = *vanguardProfile;
		invalidEnemyGrowth.profileId = "EnemyCombat.Test.InvalidGrowth";
		invalidEnemyGrowth.progression.naturalGrowth.push_back({ vega::OwnerAttributeIds::AttackPower, 1.f });
		std::string invalidGrowthError;
		if (vega::content::EnemyCombatProfileCatalog::ValidateProfile(invalidEnemyGrowth, &invalidGrowthError) || invalidGrowthError.find("Owner.MaxHealth") == std::string::npos)
		{
			return Fail("Enemy catalog accepted a non-whitelisted natural growth attribute") ? 0 : 1;
		}
		std::string legacyKeyError;
		const bool legacyKeyLoaded =
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(legacyKeyPath, &legacyKeyError);
		std::filesystem::remove(legacyKeyPath);
		if (legacyKeyLoaded || legacyKeyError.find("Unknown key 'primaryWeaponId'") == std::string::npos)
		{
			return Fail("Enemy combat profile loader accepted removed primaryWeaponId") ? 0 : 1;
		}

		// Catalog validation is generic and must not require a hardcoded roster.
		std::string shippedValErr;
		if (!vega::content::EnemyCombatProfileCatalog::ValidateShippedProfiles(&shippedValErr))
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(multiAbilityPath, &multiAbilityError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(noAttackPath, &noAttackError);
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
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(contactOnlyPath, &contactOnlyError);
		std::filesystem::remove(contactOnlyPath);
		if (!contactOnlyLoaded)
		{
			return Fail(("Contact damage only profile was rejected: " + contactOnlyError).c_str()) ? 0 : 1;
		}

		// Restore original catalog
		vega::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath);

		// Verify that the restored shipped catalog validates successfully
		if (!vega::content::EnemyCombatProfileCatalog::ValidateShippedProfiles(&shippedValErr))
		{
			return Fail(("Shipped catalog failed validation after restoration: " + shippedValErr).c_str()) ? 0 : 1;
		}

		std::string shipCatalogError;
		if (!vega::content::ShipContentCatalog::LoadFromFile(shipsPath, &shipCatalogError))
		{
			return Fail(("Ship catalog failed to load: " + shipCatalogError).c_str()) ? 0 : 1;
		}
		std::string enemyDefinitionCatalogError;
		if (!vega::content::EnemyContentCatalog::LoadFromFiles(
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_definitions.json",
			std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_behavior_profiles.json",
			&enemyDefinitionCatalogError))
		{
			return Fail(("Enemy definition catalog failed to load: " + enemyDefinitionCatalogError).c_str()) ? 0 : 1;
		}
		if (vega::content::EnemyContentCatalog::GetDefinitions().size() != 3 ||
			!vega::content::EnemyContentCatalog::FindById(vega::EnemyIds::ApproachGunnerBasic))
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
			const bool combatLoaded = vega::content::EnemyCombatProfileCatalog::LoadFromFile(fixtureCombatPath, &fixtureError);
			const bool combinedLoaded = combatLoaded && vega::content::EnemyContentCatalog::LoadFromFiles(fixtureDefinitionsPath, fixtureBehaviorPath, &fixtureError);
			std::filesystem::remove(fixtureCombatPath);
			std::filesystem::remove(fixtureDefinitionsPath);
			std::filesystem::remove(fixtureBehaviorPath);
			vega::content::EnemyCombatProfileCatalog::LoadFromFile(enemyProfilePath);
			vega::content::EnemyContentCatalog::LoadFromFiles(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_definitions.json",
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/enemy_behavior_profiles.json");
			if (combinedLoaded != expected)
			{
				return Fail(failureText) ? 0 : 1;
			}
			return 0;
		};
		const char* combinedBehaviorHeader = R"("schemaVersion":1,"profiles":[{"id":"EnemyBehavior.Test.Combined","targetSearchRange":2000.0,"targetRefreshInterval":0.2,"desiredDistance":700.0,"minimumDistance":400.0,"maximumDistance":0.0,"movementMode":"Approach","slotRules":)";
		if (validateCombinedFixture("weapon_range", R"({"schemaVersion":1,"profiles":[{"id":"EnemyCombat.Test.Combined","weapons":[{"weaponId":"Weapon.Projectile.EnemyTwinBladeScatter.Basic","slot":"PrimaryFire"}]}]})", (std::string{ "{" } + combinedBehaviorHeader + R"([{"slot":"PrimaryFire","inputMode":"Hold","requiresTarget":true,"minimumRange":0.0,"maximumRange":601.0}]}]})").c_str(), false, "Combined validation accepted a behavior weapon range above Common.Range") != 0)
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

		const ShipDefinition* approachShip = vega::content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
		const ShipDefinition* strafeShip = vega::content::ShipContentCatalog::FindById("Ship.Enemy.StrafeSkirmisher.Basic");
		const ShipDefinition* rangeShip = vega::content::ShipContentCatalog::FindById("Ship.Enemy.RangeKeeper.Basic");
		const PrimaryWeaponDefinition* approachWeapon = vega::content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyVanguardPulse.Basic");
		const PrimaryWeaponDefinition* strafeWeapon = vega::content::WeaponContentCatalog::FindById("Weapon.Projectile.EnemyTwinBladeScatter.Basic");
		const PrimaryWeaponDefinition* rangeWeapon = vega::content::WeaponContentCatalog::FindById("Weapon.Wave.EnemyHexagonCryoPulse.Basic");
		const vega::EnemyBehaviorProfile* approachBehavior = vega::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.ApproachGunner.Basic");
		const vega::EnemyBehaviorProfile* strafeBehavior = vega::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.StrafeSkirmisher.Basic");
		const vega::EnemyBehaviorProfile* rangeBehavior = vega::content::EnemyContentCatalog::FindBehaviorById("EnemyBehavior.RangeKeeper.Basic");
		auto hasNonZeroOwnerAttribute = [](const ShipDefinition* ship, const sas::AttributeId& attributeId)
		{
			return ship && std::any_of(ship->baseOwnerAttributes.begin(), ship->baseOwnerAttributes.end(),
				[&](const OwnerAttributeBaseEntry& entry) { return entry.attributeId == attributeId && !NearlyEqual(entry.baseValue, 0.f); });
		};
		const bool enemyPowerScalingIsDisabled = approachShip && strafeShip && rangeShip &&
			NearlyEqual(approachShip->energyAttributes.baseEnergyPower, 0.f) && NearlyEqual(strafeShip->energyAttributes.baseEnergyPower, 0.f) &&
			NearlyEqual(rangeShip->energyAttributes.baseEnergyPower, 0.f) &&
			!hasNonZeroOwnerAttribute(approachShip, vega::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(strafeShip, vega::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(rangeShip, vega::OwnerAttributeIds::AttackPower) &&
			!hasNonZeroOwnerAttribute(approachShip, vega::OwnerAttributeIds::EnergyPower) &&
			!hasNonZeroOwnerAttribute(strafeShip, vega::OwnerAttributeIds::EnergyPower) &&
			!hasNonZeroOwnerAttribute(rangeShip, vega::OwnerAttributeIds::EnergyPower);
		const bool enemyStatsMatchRoles = approachShip && strafeShip && rangeShip &&
			NearlyEqual(approachShip->health, 60.f) && NearlyEqual(strafeShip->health, 60.f) && NearlyEqual(rangeShip->health, 100.f) &&
			NearlyEqual(approachShip->speed.x, 0.f) && NearlyEqual(approachShip->speed.y, 0.f) &&
			NearlyEqual(strafeShip->speed.x, 0.f) && NearlyEqual(strafeShip->speed.y, 0.f) &&
			NearlyEqual(rangeShip->speed.x, 0.f) && NearlyEqual(rangeShip->speed.y, 0.f) &&
			NearlyEqual(approachShip->collisionDamage, 50.f) && NearlyEqual(strafeShip->collisionDamage, 50.f) && NearlyEqual(rangeShip->collisionDamage, 60.f) &&
			NearlyEqual(approachShip->movementAttributes.forwardThrust.currentValue, 630.f) &&
			NearlyEqual(approachShip->movementAttributes.reverseThrust.currentValue, 165.f) &&
			NearlyEqual(approachShip->movementAttributes.strafeThrust.currentValue, 240.f) &&
			NearlyEqual(approachShip->movementAttributes.maxSpeed.currentValue, 490.f) &&
			NearlyEqual(strafeShip->movementAttributes.forwardThrust.currentValue, 480.f) &&
			NearlyEqual(strafeShip->movementAttributes.reverseThrust.currentValue, 330.f) &&
			NearlyEqual(strafeShip->movementAttributes.strafeThrust.currentValue, 600.f) &&
			NearlyEqual(strafeShip->movementAttributes.maxSpeed.currentValue, 515.f) &&
			NearlyEqual(rangeShip->movementAttributes.forwardThrust.currentValue, 360.f) &&
			NearlyEqual(rangeShip->movementAttributes.reverseThrust.currentValue, 450.f) &&
			NearlyEqual(rangeShip->movementAttributes.strafeThrust.currentValue, 360.f) &&
			NearlyEqual(rangeShip->movementAttributes.maxSpeed.currentValue, 460.f);
		const bool behaviorMatchesRoles = approachBehavior && strafeBehavior && rangeBehavior &&
			approachBehavior->movementMode == vega::EnemyMovementMode::Approach &&
			strafeBehavior->movementMode == vega::EnemyMovementMode::Strafe &&
			rangeBehavior->movementMode == vega::EnemyMovementMode::HoldRange &&
			NearlyEqual(approachBehavior->desiredDistance, 800.f) && NearlyEqual(approachBehavior->minimumDistance, 400.f) &&
			NearlyEqual(strafeBehavior->minimumDistance, 300.f) && NearlyEqual(strafeBehavior->maximumDistance, 500.f) &&
			NearlyEqual(rangeBehavior->minimumDistance, 800.f) && NearlyEqual(rangeBehavior->maximumDistance, 1200.f) &&
			NearlyEqual(approachBehavior->slotRules.front().maximumRange, 1300.f) &&
			NearlyEqual(strafeBehavior->slotRules.front().maximumRange, 600.f) &&
			NearlyEqual(rangeBehavior->slotRules.front().maximumRange, 1200.f);
		const bool weaponsMatchRoles = approachWeapon && strafeWeapon && rangeWeapon &&
			approachWeapon->weaponType == PrimaryWeaponType::ProjectileStandard &&
			strafeWeapon->weaponType == PrimaryWeaponType::ProjectileShotgun &&
			rangeWeapon->weaponType == PrimaryWeaponType::WaveExpanding &&
			approachWeapon->damageTags == vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Energy } &&
			strafeWeapon->damageTags == vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Kinetic } &&
			rangeWeapon->damageTags == vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Cryo } &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, vega::CommonAttributeIds::Damage)->baseValue, 15.f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, vega::CommonAttributeIds::Damage)->baseValue, 8.f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, vega::CommonAttributeIds::Damage)->baseValue, 10.f) &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, vega::CommonAttributeIds::FireRate)->baseValue, 1.8f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, vega::CommonAttributeIds::FireRate)->baseValue, 1.2f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, vega::CommonAttributeIds::FireRate)->baseValue, 0.9f) &&
			NearlyEqual(sas::FindAttribute(approachWeapon->attributes, vega::CommonAttributeIds::Range)->baseValue, 1300.f) &&
			NearlyEqual(sas::FindAttribute(strafeWeapon->attributes, vega::CommonAttributeIds::Range)->baseValue, 600.f) &&
			NearlyEqual(sas::FindAttribute(rangeWeapon->attributes, vega::CommonAttributeIds::Range)->baseValue, 1400.f);
		const auto hasMatchingRule = [](const vega::EnemyCombatProfile* combat, const vega::EnemyBehaviorProfile* behavior)
		{
			if (!combat || !behavior || combat->weapons.size() + combat->abilities.size() != behavior->slotRules.size()) return false;
			for (const vega::EnemyWeaponBinding& binding : combat->weapons)
				if (std::none_of(behavior->slotRules.begin(), behavior->slotRules.end(), [&](const vega::EnemySlotDecisionRule& rule) { return rule.slot == binding.slot; })) return false;
			for (const vega::EnemyAbilityBinding& binding : combat->abilities)
				if (std::none_of(behavior->slotRules.begin(), behavior->slotRules.end(), [&](const vega::EnemySlotDecisionRule& rule) { return rule.slot == binding.slot; })) return false;
			return true;
		};
		const vega::EnemyCombatProfile* approachCombat = vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.ApproachGunner.Basic");
		const vega::EnemyCombatProfile* strafeCombat = vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.StrafeSkirmisher.Basic");
		const vega::EnemyCombatProfile* rangeCombat = vega::content::EnemyCombatProfileCatalog::FindById("EnemyCombat.RangeKeeper.Basic");
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
		const bool legacyBehaviorLoaded = vega::content::EnemyContentCatalog::LoadFromFiles(
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
		const bool unboundRuleLoaded = vega::content::EnemyContentCatalog::LoadFromFiles(
			testEnemyDefinitionsPath, testEnemyBehaviorPath, &unboundRuleError);
		std::filesystem::remove(testEnemyDefinitionsPath);
		std::filesystem::remove(testEnemyBehaviorPath);
		if (unboundRuleLoaded || unboundRuleError.find("active combat binding") == std::string::npos)
			return Fail("Behavior slot rule without an active combat binding was accepted") ? 0 : 1;

		std::cout << "[PASS] All Enemy Combat Profile content tests passed successfully!\n";
		return 0;
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	// The editor's acceptance test shares the production loaders without touching
	// source assets or replacing process-global runtime content catalogs.
	if (argc == 3 && std::string{ argv[1] } == "--content-root")
	{
		const auto data = std::filesystem::path{ argv[2] } / "content/data";
		const auto effects = vega::content::EffectLoader::LoadFromFile(data / "effects.json", EffectData::GetBuiltinGameplayEffectDefinitions());
		const auto abilities = vega::content::AbilityLoader::LoadFromFile(data / "abilities.json", AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions());
		const auto modules = vega::content::ModuleLoader::LoadFromFile(data / "modules.json");
		for (const auto& error : { effects.error, abilities.error, modules.error }) if (!error.empty()) return Fail(error.c_str()) ? 0 : 1;
		bool editedEffect = false, editedAbility = false;
		for (const auto& effect : effects.definitions) if (effect.id == "Effect.Authoring.Test.Renamed")
			editedEffect = NearlyEqual(effect.definition.duration, 3.5f) && !effect.definition.modifiers.empty() && NearlyEqual(effect.definition.modifiers.front().magnitude, 9.f);
		for (const auto& ability : abilities.definitions) if (ability.id == "Ability.Movement.Dash.Authoring")
			editedAbility = !ability.definition.effectSpecs.empty() && ability.definition.effectSpecs.front().effectId.ToString() == "Effect.Authoring.Test.Renamed";
		if (!editedEffect || !editedAbility) return Fail("Authored definitions did not reach the game loader") ? 0 : 1;
		std::cout << "Authored ability, effect and module files accepted by game loaders\n";
		return 0;
	}
	const int damageStatusBalanceResult = RunDamageStatusBalanceContentTests();
	if (damageStatusBalanceResult != 0)
	{
		return damageStatusBalanceResult;
	}
	const int failedReloadResult = RunFailedReloadContentTests();
	if (failedReloadResult != 0)
	{
		return failedReloadResult;
	}
	const int enemyProfileResult = RunEnemyCombatProfileContentTests();
	if (enemyProfileResult != 0)
	{
		return enemyProfileResult;
	}
	const std::filesystem::path abilityPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/abilities.json";
	const vega::List<const vega::GameAbilityDefinition*>& fallbackAbilities =
		AbilityData::GetBuiltinShippedAbilityDefinitions();
	const vega::List<const vega::AbilityActorDefinition*>& fallbackAbilityActors =
		AbilityData::GetBuiltinAbilityActorDefinitions();
	const vega::content::AbilityLoader::Result loadedAbilities =
		vega::content::AbilityLoader::LoadFromFile(
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

	const vega::content::AbilityLoader::LoadedDefinition* railBurstLoaded = nullptr;
	for (const vega::content::AbilityLoader::LoadedDefinition& loaded : loadedAbilities.definitions)
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
	if (!NearlyEqual(
			sas::FindAttributeValue(
				railBurstLoaded->actorDefinitions.front().attributes,
				vega::CommonAttributeIds::PierceDamageLoss,
				0.f
			),
			0.1f) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				railBurstLoaded->actorDefinitions.front().attributes,
				AbilityData::RailBurst::Actor::Projectile::MinimumDamageMultiplier,
				0.f
			),
			0.6f))
	{
		return Fail("Rail Burst pierce damage falloff or minimum multiplier is invalid") ? 0 : 1;
	}

	const auto astralSurgeIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.AstralSurge.Basic";
		}
	);
	if (astralSurgeIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(astralSurgeIt->definition.cooldown, 14.f) ||
		!NearlyEqual(astralSurgeIt->definition.duration, 1.f) ||
		astralSurgeIt->definition.maxCharges != 1 ||
		astralSurgeIt->definition.scalingRules.size() != 1 ||
		astralSurgeIt->definition.scalingRules.front().sourceAttributeId !=
			vega::OwnerAttributeIds::EnergyPower ||
		!NearlyEqual(astralSurgeIt->definition.scalingRules.front().coefficient, 0.6f) ||
		!astralSurgeIt->definition.ResolveLevelStep(0) ||
		astralSurgeIt->actorDefinitions.size() != 1 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				astralSurgeIt->actorDefinitions.front().attributes,
				vega::AreaAttributeIds::Width,
				0.f
			),
			280.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				astralSurgeIt->actorDefinitions.front().attributes,
				vega::CommonAttributeIds::PierceDamageLoss,
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
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.CrescentReaver.Basic";
		}
	);
	const vega::SpawnActorAction* crescentSpawn = nullptr;
	if (crescentReaverIt != loadedAbilities.definitions.end())
	{
		for (const vega::AbilityActionSpec& action : crescentReaverIt->definition.actions)
		{
			if (const auto* spawn = std::get_if<vega::SpawnActorAction>(&action.action))
			{
				crescentSpawn = spawn;
				break;
			}
		}
	}
	if (crescentReaverIt == loadedAbilities.definitions.end() ||
		crescentReaverIt->definition.scalingRules.size() != 2 ||
		crescentReaverIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Kinetic } ||
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
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Movement.EnergySpear.Basic";
		}
	);
	if (energySpearIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(energySpearIt->definition.cooldown, 10.f) ||
		!NearlyEqual(energySpearIt->definition.duration, 1.5f) ||
		energySpearIt->definition.attributes.size() != 10 ||
		!energySpearIt->definition.ResolveLevelStep(0) ||
		energySpearIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Energy } ||
		!sas::FindAttribute(
			energySpearIt->definition.attributes,
			vega::CommonAttributeIds::Radius
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				energySpearIt->definition.attributes,
				vega::CommonAttributeIds::Range,
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
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Utility.FrostMaelstrom.Basic";
		}
	);
	if (frostMaelstromIt == loadedAbilities.definitions.end() ||
		frostMaelstromIt->definition.attributes.size() != 10 ||
		!frostMaelstromIt->definition.ResolveLevelStep(0) ||
		!NearlyEqual(frostMaelstromIt->definition.duration, 6.f) ||
		frostMaelstromIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Cryo } ||
		frostMaelstromIt->actorDefinitions.size() != 1 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				frostMaelstromIt->actorDefinitions.front().attributes,
				vega::CommonAttributeIds::Duration,
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
				AbilityData::FrostMaelstrom::Attribute::PullStrength,
				0.f
			),
			300.f
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
	const vega::content::AbilityLoader::Result duplicateAbilities =
		vega::content::AbilityLoader::LoadFromFile(
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
	const vega::content::AbilityLoader::Result malformedAbilityId =
		vega::content::AbilityLoader::LoadFromFile(
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
	const vega::content::AbilityLoader::Result abilityVariant =
		vega::content::AbilityLoader::LoadFromFile(
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
	const vega::content::AbilityLoader::Result abilityScopedAttributes =
		vega::content::AbilityLoader::LoadFromFile(
			abilityScopedAttributesPath,
			fallbackAbilities,
			fallbackAbilityActors
		);
	std::filesystem::remove(abilityScopedAttributesPath);
	if (!abilityScopedAttributes.Succeeded() ||
		abilityScopedAttributes.definitions.size() != 1 ||
		sas::FindAttributeValue(
			abilityScopedAttributes.definitions.front().definition.attributes,
			vega::CommonAttributeIds::ProjectileCount
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
	const vega::content::AbilityLoader::Result missingAbilityNumbers =
		vega::content::AbilityLoader::LoadFromFile(
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
		!NearlyEqual(dash.definition.cooldown, 9.f) ||
		!NearlyEqual(dash.definition.duration, 0.24f) ||
		dash.definition.maxCharges != 1 ||
		!dash.definition.ResolveLevelStep(0) ||
		!dash.definition.ResolveLevelStep(30) ||
		dash.definition.GetMaxLevel() != vega::GameAbilityDefinition::kUnboundedAbilityLevel ||
		dash.definition.scrapCost.base != 40u ||
		dash.definition.scrapCost.step != 10u ||
		dash.definition.GetScrapCostToReachLevel(2) != 40u ||
		dash.definition.GetScrapCostToReachLevel(5) != 70u ||
		dash.definition.GetScrapCostToReachLevel(20) != 220u ||
		dash.definition.GetScrapCostToReachLevel(200) != 2020u ||
		dash.numericSettings.at("baseDistance") != 260.f ||
		dash.numericSettings.at("cameraZoomOutRatio") != 0.15f)
	{
		return Fail("Dash ability JSON profile is invalid") ? 0 : 1;
	}
	const auto shieldIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Defense.Shield.Basic";
		}
	);
	if (shieldIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(shieldIt->definition.cooldown, 10.f) ||
		!NearlyEqual(shieldIt->definition.duration, 5.f) ||
		shieldIt->definition.maxCharges != 1 ||
		shieldIt->definition.scalingRules.size() != 1 ||
		shieldIt->definition.scalingRules.front().targetAttributeId !=
			BarrierEffectSchema::Capacity ||
		!NearlyEqual(shieldIt->definition.scalingRules.front().coefficient, 0.2f) ||
		shieldIt->definition.effectSpecs.size() != 1 ||
		!shieldIt->definition.effectSpecs.front().useAbilityDuration ||
		!NearlyEqual(
			sas::FindAttributeValue(
				shieldIt->definition.effectSpecs.front().attributes,
				BarrierEffectSchema::Capacity
			),
			60.f
		))
	{
		return Fail("Shield ability JSON profile is invalid") ? 0 : 1;
	}
	const auto sunBeamIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.SunBeam.Strike.Basic";
		}
	);
	if (sunBeamIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(sunBeamIt->definition.cooldown, 9.f) ||
		!sunBeamIt->definition.ResolveLevelStep(0) ||
		sunBeamIt->definition.scrapCost.base != 40u ||
		sunBeamIt->definition.scrapCost.step != 10u ||
		sunBeamIt->definition.GetScrapCostToReachLevel(20) != 220u)
	{
		return Fail("Sun Beam ability JSON profile is invalid") ? 0 : 1;
	}
	{
		// Unbounded linear growth: every step index carries the same damage increment.
		float firstIncrement = 0.f;
		bool hasFirstIncrement = false;
		for (const std::size_t stepIndex : { std::size_t{ 0 }, std::size_t{ 3 }, std::size_t{ 23 }, std::size_t{ 98 } })
		{
			const vega::AbilityLevelStep* step = sunBeamIt->definition.ResolveLevelStep(stepIndex);
			if (!step)
			{
				return Fail("Sun Beam progression does not resolve a step at every level") ? 0 : 1;
			}
			float increment = 0.f;
			bool foundDamage = false;
			for (const sas::AttributeModifier& modifier : step->attributeModifiers)
			{
				if (modifier.attributeId == vega::CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add)
				{
					increment = modifier.magnitude;
					foundDamage = true;
				}
			}
			if (!foundDamage || (hasFirstIncrement && !NearlyEqual(increment, firstIncrement)))
			{
				return Fail("Sun Beam per-level damage increment is not constant") ? 0 : 1;
			}
			firstIncrement = increment;
			hasFirstIncrement = true;
		}
	}
	const auto rocketIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.Rocket.Basic";
		}
	);
	if (rocketIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(rocketIt->definition.cooldown, 6.f) ||
		!rocketIt->definition.ResolveLevelStep(0) ||
		!rocketIt->definition.scrapCost.IsPurchasable())
	{
		return Fail("Rocket ability JSON profile is invalid") ? 0 : 1;
	}
	const auto gravityIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Utility.GravityAnomaly.Basic";
		}
	);
	if (gravityIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(gravityIt->definition.cooldown, 13.f) ||
		!gravityIt->definition.ResolveLevelStep(0) ||
		gravityIt->definition.ResolveLevelStep(0)->attributeModifiers.size() != 2 ||
		!gravityIt->definition.scrapCost.IsPurchasable() ||
		gravityIt->actorDefinitions.size() != 2 ||
		gravityIt->actorDefinitions.front().attributes.size() != 7 ||
		gravityIt->actorDefinitions.back().attributes.size() != 5 ||
		!NearlyEqual(
			sas::FindAttributeValue(
				gravityIt->actorDefinitions.front().attributes,
				vega::CommonAttributeIds::Radius
			),
			320.f
		))
	{
		return Fail("Gravity Anomaly ability JSON profile is invalid") ? 0 : 1;
	}
	const auto infernoIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
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
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.OverdriveCore.Basic";
		}
	);
	if (overdriveIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(overdriveIt->definition.cooldown, 10.f) ||
		!NearlyEqual(overdriveIt->definition.duration, 6.f) ||
		overdriveIt->definition.attributes.size() != 4 ||
		overdriveIt->actorDefinitions.size() != 1 ||
		overdriveIt->actorDefinitions.front().presentationProfileId !=
			vega::OverdriveCorePresentationIds::ProjectileBasic)
	{
		return Fail("Overdrive Core ability JSON profile is invalid") ? 0 : 1;
	}
	const auto nullPulseIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Utility.NullPulse.Basic";
		}
	);
	if (nullPulseIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(nullPulseIt->definition.cooldown, 12.f) ||
		nullPulseIt->definition.duration != 0.f ||
		nullPulseIt->definition.maxCharges != 1 ||
		nullPulseIt->definition.attributes.size() != 4 ||
		!nullPulseIt->definition.ResolveLevelStep(0) ||
		nullPulseIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Energy })
	{
		return Fail("Null Pulse ability JSON profile is invalid") ? 0 : 1;
	}
	const auto phaseDriftIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Movement.PhaseDrift.Basic";
		}
	);
	if (phaseDriftIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(phaseDriftIt->definition.cooldown, 16.f) ||
		!NearlyEqual(phaseDriftIt->definition.duration, 3.5f) ||
		phaseDriftIt->definition.maxCharges != 1 ||
		phaseDriftIt->definition.attributes.size() != 2 ||
		!phaseDriftIt->definition.ResolveLevelStep(0))
	{
		return Fail("Phase Drift ability JSON profile is invalid") ? 0 : 1;
	}
	const auto shieldHarvestIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Defense.ShieldHarvest.Basic";
		}
	);
	if (shieldHarvestIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(shieldHarvestIt->definition.cooldown, 14.f) ||
		!NearlyEqual(shieldHarvestIt->definition.duration, 1.5f) ||
		shieldHarvestIt->definition.maxCharges != 1 ||
		shieldHarvestIt->definition.attributes.size() != 3 ||
		!shieldHarvestIt->definition.ResolveLevelStep(0) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				shieldHarvestIt->definition.attributes,
				vega::CommonAttributeIds::Radius
			),
			700.f
		))
	{
		return Fail("Shield Harvest ability JSON profile is invalid") ? 0 : 1;
	}
	const auto hullShockIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
		{
			return definition.id == "Ability.Offense.HullShock.Basic";
		}
	);
	if (hullShockIt == loadedAbilities.definitions.end() ||
		!NearlyEqual(hullShockIt->definition.cooldown, 12.f) ||
		!NearlyEqual(hullShockIt->definition.duration, 2.f) ||
		hullShockIt->definition.maxCharges != 1 ||
		hullShockIt->definition.attributes.size() != 6 ||
		hullShockIt->definition.scalingRules.size() != 1 ||
		!hullShockIt->definition.ResolveLevelStep(0) ||
		hullShockIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Electric } ||
		!NearlyEqual(
			sas::FindAttributeValue(
				hullShockIt->definition.attributes,
				vega::CommonAttributeIds::Radius
			),
			600.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				hullShockIt->definition.attributes,
				vega::CommonAttributeIds::Damage
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
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
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
			vega::List<vega::GameplayTag>{
				vega::GameplayTags::Ability::Offense,
				vega::GameplayTags::Ability::Family::OrbitalDrones
			} ||
		orbitalDronesIt->definition.attributes.size() != 6 ||
		sas::FindAttributeValue(
			orbitalDronesIt->definition.attributes,
			vega::CommonAttributeIds::Radius,
			0.f
		) <= 0.f ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesIt->definition.attributes,
				vega::CommonAttributeIds::Damage,
				0.f
			),
			25.f
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
		orbitalDronesIt->definition.scalingRules.size() != 1 ||
		orbitalDronesIt->definition.scalingRules.front().targetAttributeId !=
			vega::CommonAttributeIds::Damage ||
		orbitalDronesIt->definition.scalingRules.front().sourceAttributeId !=
			vega::OwnerAttributeIds::AttackPower ||
		orbitalDronesIt->definition.scalingRules.front().operation !=
			sas::AttributeModifierOperation::Add ||
		!NearlyEqual(orbitalDronesIt->definition.scalingRules.front().coefficient, 0.40f) ||
		!orbitalDronesIt->definition.ResolveLevelStep(0) ||
		orbitalDronesIt->definition.scrapCost.base != 60u ||
		orbitalDronesIt->definition.damageTags !=
			vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Kinetic })
	{
		return Fail("Orbital Drones ability JSON profile is invalid") ? 0 : 1;
	}
	for (const std::size_t stepIndex : { std::size_t{ 0 }, std::size_t{ 10 }, std::size_t{ 50 } })
	{
		const vega::AbilityLevelStep* step = orbitalDronesIt->definition.ResolveLevelStep(stepIndex);
		if (!step || step->attributeModifiers.size() != 1)
		{
			return Fail("Orbital Drones level progression does not contain exactly one modifier")
				? 0
				: 1;
		}
		const sas::AttributeModifier& modifier = step->attributeModifiers.front();
		if (modifier.attributeId != vega::CommonAttributeIds::Damage ||
			modifier.operation != sas::AttributeModifierOperation::Add ||
			!NearlyEqual(modifier.magnitude, 4.f))
		{
			return Fail("Orbital Drones level progression has an unexpected modifier") ? 0 : 1;
		}
	}
	if (orbitalDronesIt->definition.GetScrapCostToReachLevel(30) != 60u)
	{
		return Fail("Orbital Drones scrap cost must keep following the linear formula at high levels") ? 0 : 1;
	}

	const auto relayPrismIt = std::find_if(
		loadedAbilities.definitions.begin(),
		loadedAbilities.definitions.end(),
		[](const vega::content::AbilityLoader::LoadedDefinition& definition)
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
				vega::CommonAttributeIds::ProjectileCount,
				0.f
			),
			4.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				relayPrismIt->definition.attributes,
				AbilityData::RelayPrism::Attribute::BaseTransfer,
				0.f
			),
			0.2f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				relayPrismIt->actorDefinitions.front().attributes,
				vega::CommonAttributeIds::Radius,
				0.f
			),
			100.f
		) ||
		!relayPrismIt->definition.ResolveLevelStep(0) ||
		!relayPrismIt->definition.scrapCost.IsPurchasable())
	{
		return Fail("Relay Prism ability JSON profile is invalid") ? 0 : 1;
	}

	const std::filesystem::path weaponPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/weapons.json";

	const vega::content::WeaponLoader::Result loaded =
		vega::content::WeaponLoader::LoadFromFile(weaponPath);
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
		"Weapon.Cone.RapidShotgun.Basic",
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
		vega::CommonAttributeIds::Damage
	);
	if (basicLaser->weaponType != PrimaryWeaponType::ProjectileStandard ||
		!basicDamage ||
		!NearlyEqual(basicDamage->baseValue, 10.f) ||
		!basicLaser->empoweredShot.has_value() ||
		basicLaser->empoweredShot->trigger != PrimaryWeaponEmpoweredTrigger::EverySuccessfulHits ||
		basicLaser->progressionProfile.ResolveProgression().prefix.size() != 0 ||
		basicLaser->progressionProfile.ResolveProgression().cycle.size() != 1 ||
		basicLaser->progressionProfile.GetScrapCostToReachLevel(200) != 1030u ||
		basicLaser->damageTags != vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Photonic })
	{
		return Fail("Basic rapid laser JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* dual =
		FindWeapon(loaded.definitions, "Weapon.Projectile.DualKineticBlaster.Basic");
	if (dual->muzzleDefinitions.size() != 2 ||
		dual->progressionProfile.ResolveDistinctSteps().empty() ||
		dual->damageTags != vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Photonic })
	{
		return Fail("Dual kinetic blaster JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* electric =
		FindWeapon(loaded.definitions, "Weapon.Arc.ElectricLauncher.Basic");
	if (electric->weaponType != PrimaryWeaponType::ArcElectric ||
		!sas::FindAttribute(
			electric->attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainCount
		))
	{
		return Fail("Electric arc launcher JSON profile is invalid") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* beam =
		FindWeapon(loaded.definitions, "Weapon.Beam.ContinuousHeatLaser.Basic");
	const PrimaryWeaponDefinition* enemyEnergy =
		FindWeapon(loaded.definitions, "Weapon.Projectile.EnemyVanguardPulse.Basic");
	if (beam->weaponType != PrimaryWeaponType::BeamContinuous ||
		beam->featureTypes != vega::List<PrimaryWeaponFeatureType>{ PrimaryWeaponFeatureType::Heat } ||
		beam->heatGainCurve.size() != 4 ||
		beam->heatDamageRegions.empty() ||
		beam->heatDamageRegions.back().damageTags != vega::List<vega::GameplayTag>{ vega::DamageTypeSchema::Energy } ||
		!enemyEnergy ||
		sas::FindAttribute(beam->attributes, vega::DamageAttributeIds::ShieldDamageMultiplier) ||
		sas::FindAttribute(enemyEnergy->attributes, vega::DamageAttributeIds::ShieldDamageMultiplier) ||
		!sas::FindAttribute(beam->attributes, vega::DamageAttributeIds::ShieldRegenerationDelay) ||
		!sas::FindAttribute(enemyEnergy->attributes, vega::DamageAttributeIds::ShieldRegenerationDelay))
	{
		return Fail("Shipped Energy weapon JSON profiles must omit the baseline multiplier and retain shield regeneration delay") ? 0 : 1;
	}

	const PrimaryWeaponDefinition* cryo =
		FindWeapon(loaded.definitions, "Weapon.Wave.CryoProjector.Basic");
	if (cryo->weaponType != PrimaryWeaponType::WaveExpanding)
	{
		return Fail("Cryo wave projector JSON profile is invalid") ? 0 : 1;
	}

	const std::filesystem::path shipPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/ships.json";
	const ShipDefinition shipPresentationBase{
		"SpaceShooterRedux/PNG/playerShip1_blue.png",
		100.f,
		sf::Vector2f{ 350.f, 350.f },
		25.f,
		0.f,
		0,
		{},
		{}
	};
	const vega::content::ShipLoader::Result loadedShips =
		vega::content::ShipLoader::LoadFromFile(
			shipPath,
			shipPresentationBase
		);
	if (!loadedShips.Succeeded())
	{
		return Fail("Ship JSON catalog did not load the player definition") ? 0 : 1;
	}
	if (ValidateShippedPlayerRoster(shipPath, loadedShips) != 0)
	{
		return 1;
	}

	const std::filesystem::path effectPath =
		std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } /
		"LightYearsGame/assets/content/data/effects.json";
	const vega::content::EffectLoader::Result loadedEffects =
		vega::content::EffectLoader::LoadFromFile(
			effectPath,
		EffectData::GetBuiltinGameplayEffectDefinitions()
	);
	const auto barrierEffectIt = std::find_if(
		loadedEffects.definitions.begin(),
		loadedEffects.definitions.end(),
		[](const vega::content::EffectLoader::LoadedDefinition& definition)
		{
			return definition.id == "Effect.Barrier.Basic";
		}
	);
	if (!loadedEffects.Succeeded())
	{
		return Fail(loadedEffects.error.c_str()) ? 0 : 1;
	}
	if (loadedEffects.definitions.size() != 21 ||
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
	const vega::content::EffectLoader::Result invalidOwnedEffect =
		vega::content::EffectLoader::LoadFromFile(
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
	const vega::content::EffectLoader::Result invalidEffectTags =
		vega::content::EffectLoader::LoadFromFile(
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
	const vega::content::EffectLoader::Result malformedEffectId =
		vega::content::EffectLoader::LoadFromFile(
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
	if (!vega::content::EffectContentCatalog::LoadFromFile(
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
		vega::content::EffectContentCatalog::LoadFromFile(
			partialEffectPath,
			EffectData::GetBuiltinGameplayEffectDefinitions(),
			&partialEffectCatalogFailure
		);
	std::filesystem::remove(partialEffectPath);
	if (!partialEffectCatalogLoaded ||
		vega::content::EffectContentCatalog::FindById("Effect.Barrier.Basic") == nullptr ||
		vega::content::EffectContentCatalog::FindById("Effect.Status.Damage.Ignite") != nullptr)
	{
		return Fail("Effect catalog reintroduced a C++ fallback record missing from JSON") ? 0 : 1;
	}

	std::cout << "Ability, weapon, ship, effect, and enemy combat profile content loader tests passed\n";
	return 0;
}
