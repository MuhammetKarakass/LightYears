#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/enemy/EnemyBehaviorProfile.h"
#include "gameplay/enemy/EnemyCombatProfile.h"
#include "gameplay/enemy/EnemySpawnContext.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/enemy/EnemyRuntime.h"
#include "enemy/EnemyActor.h"
#include "level/ArenaLevel.h"
#include "level/ArenaTestLevel.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	using Json = nlohmann::json;

	class HeadlessShipLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessShipLevel(ly::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	class ShipSelectionArena final : public ly::ArenaTestLevel
	{
	public:
		explicit ShipSelectionArena(ly::Application* application) : ArenaTestLevel{ application } {}

		bool DispatchKey(sf::Keyboard::Key key)
		{
			return DispatchEvent(sf::Event{ sf::Event::KeyPressed{ key } });
		}

	protected:
		void CreateHUDControllers() override {}
		void OnGameStart() override { ArenaLevel::OnGameStart(); }
	};

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{
			{ "test", "PlayerShipSelectionE2E" },
			{ "passed", false },
			{ "checks", Json::array() }
		};

		void Check(bool passed, const std::string& name, Json actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = std::move(actual);
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.0001f;
	}

	float ReadAttribute(const sas::AttributeSystem& attributes, const sas::AttributeId& id)
	{
		return attributes.GetCurrentValue(id);
	}

	float ReadGrowth(const ShipDefinition& definition, const sas::AttributeId& id)
	{
		for (const AttributeGrowthEntry& entry : definition.progressionDefinition.naturalGrowth)
		{
			if (entry.attributeId == id) return entry.perLevel;
		}
		return 0.f;
	}

	float ReadOwnerBaseValue(const ShipDefinition& definition, const sas::AttributeId& id)
	{
		if (id == ly::OwnerAttributeIds::MaxHealth) return definition.health;
		if (id == ly::OwnerAttributeIds::EnergyPower) return definition.energyAttributes.baseEnergyPower;
		for (const OwnerAttributeBaseEntry& entry : definition.baseOwnerAttributes)
		{
			if (entry.attributeId == id) return entry.baseValue;
		}
		return 0.f;
	}

	void CheckShipLevel(Report& report, ly::PlayerSpaceShip& ship, const ShipDefinition& definition, int level)
	{
		sas::AttributeSystem& ownerAttributes = ship.GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = ship.GetShipRuntime().GetAttributes();
		const float expectedShield = definition.energyAttributes.baseMaxShield +
			definition.energyAttributes.maxShieldPerLevel * static_cast<float>(level - 1);
		const float expectedAfterburner = definition.energyAttributes.baseAfterburnerCapacity +
			definition.energyAttributes.afterburnerCapacityPerLevel * static_cast<float>(level - 1);
		const float expectedEnergyPower = ReadAttribute(ownerAttributes, ly::OwnerAttributeIds::EnergyPower);
		const float actualShield = ReadAttribute(shipAttributes, ly::ShipAttributeIds::MaxShield);
		const float actualAfterburner = ReadAttribute(shipAttributes, ly::ShipAttributeIds::AfterburnerCapacity);
		const float expectedLevelOneEnergy = definition.energyAttributes.baseEnergyPower +
			ReadGrowth(definition, ly::OwnerAttributeIds::EnergyPower) * static_cast<float>(level - 1);
		report.Check(NearlyEqual(actualShield, expectedShield), definition.primaryWeaponId + " shield capacity at level " + std::to_string(level),
			{ { "actual", actualShield }, { "expected", expectedShield }, { "energyPower", expectedEnergyPower } });
		report.Check(NearlyEqual(actualAfterburner, expectedAfterburner), definition.primaryWeaponId + " afterburner capacity at level " + std::to_string(level),
			{ { "actual", actualAfterburner }, { "expected", expectedAfterburner }, { "energyPower", expectedEnergyPower } });
		report.Check(NearlyEqual(expectedEnergyPower, expectedLevelOneEnergy), definition.primaryWeaponId + " energy power progression at level " + std::to_string(level),
			{ { "actual", expectedEnergyPower }, { "expected", expectedLevelOneEnergy } });

		const std::array<std::pair<const sas::AttributeId*, const char*>, 5> ownerStats{{
			{ &ly::OwnerAttributeIds::MaxHealth, "max health" },
			{ &ly::OwnerAttributeIds::AttackPower, "attack power" },
			{ &ly::OwnerAttributeIds::Armor, "armor" },
			{ &ly::OwnerAttributeIds::Luck, "luck" },
			{ &ly::OwnerAttributeIds::AttackSpeed, "attack speed" }
		}};
		for (const auto& [attributeId, label] : ownerStats)
		{
			const float expected = ReadOwnerBaseValue(definition, *attributeId) +
				ReadGrowth(definition, *attributeId) * static_cast<float>(level - 1);
			const float actual = ReadAttribute(ownerAttributes, *attributeId);
			report.Check(NearlyEqual(actual, expected), definition.primaryWeaponId + " " + label + " at level " + std::to_string(level),
				{ { "actual", actual }, { "expected", expected } });
		}

		const sas::AttributeModifierHandle energyProbe = ownerAttributes.AddModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::EnergyPower, 50.f }
		);
		const float probedEnergyPower = ReadAttribute(ownerAttributes, ly::OwnerAttributeIds::EnergyPower);
		const float probedShield = ReadAttribute(shipAttributes, ly::ShipAttributeIds::MaxShield);
		const float probedAfterburner = ReadAttribute(shipAttributes, ly::ShipAttributeIds::AfterburnerCapacity);
		ownerAttributes.RemoveModifier(energyProbe);
		report.Check(NearlyEqual(probedEnergyPower, expectedEnergyPower + 50.f) &&
			NearlyEqual(probedShield, actualShield) && NearlyEqual(probedAfterburner, actualAfterburner),
			definition.primaryWeaponId + " EP modifier leaves shield and afterburner unchanged at level " + std::to_string(level),
			{ { "energyPowerBefore", expectedEnergyPower }, { "energyPowerWithModifier", probedEnergyPower },
				{ "shieldBefore", actualShield }, { "shieldWithModifier", probedShield },
				{ "afterburnerBefore", actualAfterburner }, { "afterburnerWithModifier", probedAfterburner } });
	}

	void CheckPrimaryWeapon(
		Report& report,
		ly::PlayerSpaceShip& ship,
		const std::string& shipId,
		const ShipDefinition& definition
	)
	{
		const PrimaryWeaponDefinition* weapon = ly::content::WeaponContentCatalog::FindById(definition.primaryWeaponId);
		auto& abilities = ship.GetCombatRuntime().GetAbilitySystemComponent();
		ly::GameAbility* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		const std::string expectedAbilityId = weapon
			? AbilityData::MakePrimaryFireAbilityDefinition(*weapon).abilityId
			: std::string{};
		report.Check(weapon && primary && primary->GetDefinition().abilityId == expectedAbilityId,
			shipId + " grants its authored primary weapon ability",
			{ { "weaponId", definition.primaryWeaponId },
				{ "expectedAbilityId", expectedAbilityId },
				{ "actualAbilityId", primary ? primary->GetDefinition().abilityId : std::string{} } });
	}

	void RunPlayerRuntimeChecks(Report& report, ly::Application& application)
	{
		using namespace ly;
		const std::array<const char*, 6> shipIds{
			"Ship.Player.Fighter.Basic",
			"Ship.Player.Breacher.Basic",
			"Ship.Player.Interceptor.Basic",
			"Ship.Player.Conductor.Basic",
			"Ship.Player.Aegis.Basic",
			"Ship.Player.CryoController.Basic"
		};
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();

		for (const char* shipId : shipIds)
		{
			manager.SetSelectedShipId(shipId);
			Player& player = manager.CreateNewPlayer();
			const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition(shipId);
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(static_cast<bool>(ship), std::string{ shipId } + " spawns through Player");
			level->TickInternal(0.f);
			CheckShipLevel(report, *ship, definition, 1);
			CheckPrimaryWeapon(report, *ship, shipId, definition);

			player.AwardShipXP(definition.progressionDefinition.baseXP);
			report.Check(player.GetShipProgression().GetLevel() == 2, std::string{ shipId } + " reaches level two from one level threshold");
			level->TickInternal(0.f);
			CheckShipLevel(report, *ship, definition, 2);

			ship->Destroy();
			level->TickInternal(0.f);
			ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(static_cast<bool>(ship), std::string{ shipId } + " respawns at its current level");
			level->TickInternal(0.f);
			CheckShipLevel(report, *ship, definition, 2);
			CheckPrimaryWeapon(report, *ship, shipId, definition);

			if (std::string{ shipId } == "Ship.Player.Fighter.Basic")
			{
				player.AwardShipXP(84.f);
				level->TickInternal(0.f);
				report.Check(player.GetShipProgression().GetLevel() == 10, "Fighter reaches level ten at cumulative 90 XP",
					{ { "level", player.GetShipProgression().GetLevel() }, { "xp", player.GetShipProgression().GetXP() } });
				CheckShipLevel(report, *ship, definition, 10);
			}
			ship->Destroy();
			level->TickInternal(0.f);
		}

		manager.SetSelectedShipId("Ship.Player.Interceptor.Basic");
		manager.Reset();
		report.Check(manager.GetSelectedShipId() == "Ship.Player.Interceptor.Basic", "PlayerManager Reset preserves selected ship ID",
			manager.GetSelectedShipId());
		manager.SetSelectedShipId("Ship.Player.Unknown.Basic");
		manager.Reset();
		Player& fallbackPlayer = manager.CreateNewPlayer();
		const shared_ptr<PlayerSpaceShip> fallbackShip = fallbackPlayer.SpawnSpaceShip(level.get()).lock();
		report.Check(fallbackShip && NearlyEqual(
			ReadAttribute(fallbackShip->GetShipRuntime().GetAttributes(), ShipAttributeIds::MaxShield),
			content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic").energyAttributes.baseMaxShield),
			"unknown selected ship falls back to Fighter");
		if (fallbackShip) fallbackShip->Destroy();
		level->TickInternal(0.f);
	}

	void RunSelectionKeyChecks(Report& report, ly::Application& application)
	{
		using namespace ly;
		const std::array<sf::Keyboard::Key, 6> keys{
			sf::Keyboard::Key::F1,
			sf::Keyboard::Key::F2,
			sf::Keyboard::Key::F3,
			sf::Keyboard::Key::F4,
			sf::Keyboard::Key::F5,
			sf::Keyboard::Key::F6
		};
		const std::array<const char*, 6> shipIds{
			"Ship.Player.Fighter.Basic",
			"Ship.Player.Breacher.Basic",
			"Ship.Player.Interceptor.Basic",
			"Ship.Player.Conductor.Basic",
			"Ship.Player.Aegis.Basic",
			"Ship.Player.CryoController.Basic"
		};
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		report.Check(manager.GetSelectedShipId() == shipIds.front(), "PlayerManager defaults to Fighter", manager.GetSelectedShipId());
		manager.SetSelectedShipId(shipIds.front());
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<ShipSelectionArena>(&application);
		level->BeginPlayInternal();
		level->TickInternal(0.f);

		for (std::size_t index = 0; index < keys.size(); ++index)
		{
			Player* previousPlayer = manager.GetPlayer();
			const shared_ptr<PlayerSpaceShip> previousShip = previousPlayer
				? previousPlayer->GetCurrentSpaceShip().lock()
				: shared_ptr<PlayerSpaceShip>{};
			const auto previousShipId = previousShip ? previousShip->GetUniqueID() : 0;
			level->DispatchKey(keys[index]);
			level->TickInternal(0.f);
			Player* player = manager.GetPlayer();
			const shared_ptr<PlayerSpaceShip> ship = player ? player->GetCurrentSpaceShip().lock() : shared_ptr<PlayerSpaceShip>{};
			const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition(shipIds[index]);
			report.Check(manager.GetSelectedShipId() == shipIds[index], "F" + std::to_string(index + 1) + " selects the matching ship ID",
				manager.GetSelectedShipId());
			report.Check(ship && player && player->GetShipProgression().GetLevel() == 1,
				"F" + std::to_string(index + 1) + " restart creates a level-one player and ship");
			report.Check(previousShip && previousShip->GetIsPendingDestroy() && ship && ship->GetUniqueID() != previousShipId,
				"F" + std::to_string(index + 1) + " restart replaces the previous ship");
			if (ship)
			{
				CheckShipLevel(report, *ship, definition, 1);
				CheckPrimaryWeapon(report, *ship, shipIds[index], definition);
			}
		}

		Player* player = manager.GetPlayer();
		shared_ptr<PlayerSpaceShip> beforeSameKey = player ? player->GetCurrentSpaceShip().lock() : shared_ptr<PlayerSpaceShip>{};
		level->DispatchKey(sf::Keyboard::Key::F6);
		level->TickInternal(0.f);
		player = manager.GetPlayer();
		const shared_ptr<PlayerSpaceShip> afterSameKey = player ? player->GetCurrentSpaceShip().lock() : shared_ptr<PlayerSpaceShip>{};
		report.Check(manager.GetSelectedShipId() == shipIds.back() && beforeSameKey && beforeSameKey->GetIsPendingDestroy() &&
			afterSameKey && beforeSameKey->GetUniqueID() != afterSameKey->GetUniqueID(),
			"pressing the already selected key still restarts the run");

		const shared_ptr<PlayerSpaceShip> beforePause = afterSameKey;
		level->SetPaused(true);
		level->DispatchKey(sf::Keyboard::Key::F1);
		player = manager.GetPlayer();
		report.Check(manager.GetSelectedShipId() == shipIds.back() && player && player->GetCurrentSpaceShip().lock() == beforePause,
			"ship keys are ignored while paused");
		level->SetPaused(false);

		level->GameFinished(false);
		level->SetPaused(false);
		level->DispatchKey(sf::Keyboard::Key::F1);
		player = manager.GetPlayer();
		report.Check(manager.GetSelectedShipId() == shipIds.back() && player && player->GetCurrentSpaceShip().lock() == beforePause,
			"ship keys are ignored after game over");
	}

	void RunEnemyShieldRegression(Report& report, ly::Application& application)
	{
		using namespace ly;
		World world{ &application };
		const ShipDefinition* enemyShip = content::ShipContentCatalog::FindById("Ship.Enemy.ApproachGunner.Basic");
		report.Check(enemyShip != nullptr, "enemy shield regression uses the shipped Approach Gunner ship");
		EnemyCombatProfile profile;
		profile.profileId = "EnemyCombat.PlayerShipSelection.ShieldGrowth";
		profile.weapons.push_back({ "Weapon.Projectile.EnemyVanguardPulse.Basic", sas::AbilitySlot::PrimaryFire, 1 });
		profile.progression.maxShieldPerLevel = 2.f;
		EnemyBehaviorProfile behavior{ "EnemyBehavior.PlayerShipSelection.ShieldGrowth", 1000.f, 0.2f, 300.f, 0.f, 0.f,
			EnemyMovementMode::Approach, 1.f, 1.f };
		const shared_ptr<EnemyActor> enemy = world.SpawnActor<EnemyActor>(
			*enemyShip,
			"Enemy.PlayerShipSelection.ShieldGrowth",
			profile,
			behavior,
			1.5f,
			EnemySpawnContext{ 3, 0 }
		).lock();
		report.Check(static_cast<bool>(enemy), "real EnemyActor spawns for enemy shield regression");
		enemy->BeginPlay();
		const sas::AttributeSystem& attributes = enemy->GetShipRuntime().GetAttributes();
		const float atLevelThree = ReadAttribute(attributes, ShipAttributeIds::MaxShield);
		report.Check(enemy->GetEnemyRuntime().IsReady() && NearlyEqual(atLevelThree, 29.f),
			"enemy shield growth remains base 25 plus two per level at level three", atLevelThree);
		const bool cleared = enemy->GetEnemyRuntime().Clear();
		const float afterClear = ReadAttribute(attributes, ShipAttributeIds::MaxShield);
		report.Check(cleared && NearlyEqual(afterClear, 25.f), "enemy runtime clear removes only its shield growth", afterClear);
	}

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open player ship test artifact path" };
		output << artifact.dump(2) << '\n';
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: PlayerShipSelectionTests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Player ship selection E2E";
		ly::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunSelectionKeyChecks(report, application);
		RunPlayerRuntimeChecks(report, application);
		RunEnemyShieldRegression(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "Player ship selection E2E failed: " << exception.what() << '\n';
	}

	try
	{
		WriteArtifact(artifactPath, report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write player ship test artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
