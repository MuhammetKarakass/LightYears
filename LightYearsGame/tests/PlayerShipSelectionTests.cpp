#include "gameFramework/LightYearsAssets.h"
#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
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
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	using Json = nlohmann::json;

	class HeadlessShipLevel final : public vega::GameLevel
	{
	public:
		explicit HeadlessShipLevel(vega::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	class ShipSelectionArena final : public vega::ArenaTestLevel
	{
	public:
		explicit ShipSelectionArena(vega::Application* application) : ArenaTestLevel{ application } {}

		// A full tap: press then release, as a real key press arrives.
		bool DispatchKey(sf::Keyboard::Key key)
		{
			const bool handled = DispatchKeyPressOnly(key);
			ReleaseKey(key);
			return handled;
		}

		void ReleaseKey(sf::Keyboard::Key key)
		{
			DispatchEvent(sf::Event{ sf::Event::KeyReleased{ key } });
		}

		// A press without release, as OS key repeat delivers while held.
		bool DispatchKeyPressOnly(sf::Keyboard::Key key)
		{
			return DispatchEvent(sf::Event{ sf::Event::KeyPressed{ key } });
		}

	protected:
		void CreateHUDControllers() override {}
		void OnGameStart() override { ArenaLevel::OnGameStart(); }
	};

	struct PlayerManagerResetGuard
	{
		vega::PlayerManager& manager;
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
		if (id == vega::OwnerAttributeIds::MaxHealth) return definition.health;
		if (id == vega::OwnerAttributeIds::EnergyPower) return definition.energyAttributes.baseEnergyPower;
		for (const OwnerAttributeBaseEntry& entry : definition.baseOwnerAttributes)
		{
			if (entry.attributeId == id) return entry.baseValue;
		}
		return 0.f;
	}

	void CheckShipLevel(Report& report, vega::PlayerSpaceShip& ship, const ShipDefinition& definition, int level)
	{
		sas::AttributeSystem& ownerAttributes = ship.GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = ship.GetShipRuntime().GetAttributes();
		const float expectedShield = definition.energyAttributes.baseMaxShield +
			definition.energyAttributes.maxShieldPerLevel * static_cast<float>(level - 1);
		const float expectedAfterburner = definition.energyAttributes.baseAfterburnerCapacity +
			definition.energyAttributes.afterburnerCapacityPerLevel * static_cast<float>(level - 1);
		const float expectedEnergyPower = ReadAttribute(ownerAttributes, vega::OwnerAttributeIds::EnergyPower);
		const float actualShield = ReadAttribute(shipAttributes, vega::ShipAttributeIds::MaxShield);
		const float actualAfterburner = ReadAttribute(shipAttributes, vega::ShipAttributeIds::AfterburnerCapacity);
		const float expectedLevelOneEnergy = definition.energyAttributes.baseEnergyPower +
			ReadGrowth(definition, vega::OwnerAttributeIds::EnergyPower) * static_cast<float>(level - 1);
		report.Check(NearlyEqual(actualShield, expectedShield), definition.primaryWeaponId + " shield capacity at level " + std::to_string(level),
			{ { "actual", actualShield }, { "expected", expectedShield }, { "energyPower", expectedEnergyPower } });
		report.Check(NearlyEqual(actualAfterburner, expectedAfterburner), definition.primaryWeaponId + " afterburner capacity at level " + std::to_string(level),
			{ { "actual", actualAfterburner }, { "expected", expectedAfterburner }, { "energyPower", expectedEnergyPower } });
		report.Check(NearlyEqual(expectedEnergyPower, expectedLevelOneEnergy), definition.primaryWeaponId + " energy power progression at level " + std::to_string(level),
			{ { "actual", expectedEnergyPower }, { "expected", expectedLevelOneEnergy } });

		const std::array<std::pair<const sas::AttributeId*, const char*>, 5> ownerStats{{
			{ &vega::OwnerAttributeIds::MaxHealth, "max health" },
			{ &vega::OwnerAttributeIds::AttackPower, "attack power" },
			{ &vega::OwnerAttributeIds::Armor, "armor" },
			{ &vega::OwnerAttributeIds::Luck, "luck" },
			{ &vega::OwnerAttributeIds::AttackSpeed, "attack speed" }
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
			sas::AttributeModifier{ vega::OwnerAttributeIds::EnergyPower, 50.f }
		);
		const float probedEnergyPower = ReadAttribute(ownerAttributes, vega::OwnerAttributeIds::EnergyPower);
		const float probedShield = ReadAttribute(shipAttributes, vega::ShipAttributeIds::MaxShield);
		const float probedAfterburner = ReadAttribute(shipAttributes, vega::ShipAttributeIds::AfterburnerCapacity);
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
		vega::PlayerSpaceShip& ship,
		const std::string& shipId,
		const ShipDefinition& definition
	)
	{
		const PrimaryWeaponDefinition* weapon = vega::content::WeaponContentCatalog::FindById(definition.primaryWeaponId);
		auto& abilities = ship.GetCombatRuntime().GetAbilitySystemComponent();
		vega::GameAbility* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		const std::string expectedAbilityId = weapon
			? AbilityData::MakePrimaryFireAbilityDefinition(*weapon).abilityId
			: std::string{};
		report.Check(weapon && primary && primary->GetDefinition().abilityId == expectedAbilityId,
			shipId + " grants its authored primary weapon ability",
			{ { "weaponId", definition.primaryWeaponId },
				{ "expectedAbilityId", expectedAbilityId },
				{ "actualAbilityId", primary ? primary->GetDefinition().abilityId : std::string{} } });
	}

	void RunPlayerRuntimeChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
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

	const vega::ModulePlacement* FindInstalled(const vega::Player& player, const char* moduleId)
	{
		for (const vega::ModulePlacement& placement : player.GetModuleLoadout().GetPlacements())
		{
			if (placement.moduleId == moduleId) return &placement;
		}
		return nullptr;
	}

	void CheckEnergySystems(
		Report& report,
		vega::PlayerSpaceShip& ship,
		const ShipDefinition& definition,
		int level,
		bool shieldOn,
		bool afterburnerOn,
		const std::string& label
	)
	{
		const ShipEnergyAttributes& energy = definition.energyAttributes;
		const float levels = static_cast<float>(level - 1);
		const float maxShield = shieldOn ? energy.baseMaxShield + energy.maxShieldPerLevel * levels : 0.f;
		const float capacity = afterburnerOn ? energy.baseAfterburnerCapacity + energy.afterburnerCapacityPerLevel * levels : 0.f;
		const float shieldRegen = maxShield / std::max(0.001f, energy.shieldFullRechargeDuration);
		const float afterburnerRegen = capacity / std::max(0.001f, energy.afterburnerFullRechargeDuration);
		const sas::AttributeSystem& attributes = ship.GetShipRuntime().GetAttributes();
		auto read = [&attributes](const sas::AttributeId& id) { return ReadAttribute(attributes, id); };
		report.Check(NearlyEqual(read(vega::ShipAttributeIds::MaxShield), maxShield), label + " MaxShield",
			{ { "actual", read(vega::ShipAttributeIds::MaxShield) }, { "expected", maxShield } });
		report.Check(NearlyEqual(read(vega::ShipAttributeIds::ShieldRegen), shieldRegen), label + " ShieldRegen",
			{ { "actual", read(vega::ShipAttributeIds::ShieldRegen) }, { "expected", shieldRegen } });
		report.Check(NearlyEqual(read(vega::ShipAttributeIds::AfterburnerCapacity), capacity), label + " AfterburnerCapacity",
			{ { "actual", read(vega::ShipAttributeIds::AfterburnerCapacity) }, { "expected", capacity } });
		report.Check(NearlyEqual(read(vega::ShipAttributeIds::AfterburnerRegen), afterburnerRegen), label + " AfterburnerRegen",
			{ { "actual", read(vega::ShipAttributeIds::AfterburnerRegen) }, { "expected", afterburnerRegen } });
		report.Check(NearlyEqual(ship.GetShieldComponent().GetMaxShield(), maxShield), label + " live shield max follows");
		report.Check(NearlyEqual(ship.GetEnergyComponent().GetMaxEnergy(), capacity), label + " live afterburner energy max follows");
		if (!shieldOn) report.Check(NearlyEqual(ship.GetShieldComponent().GetShield(), 0.f), label + " live shield clamps to 0");
		if (!afterburnerOn)
		{
			report.Check(ship.GetEnergyComponent().Consume(1.f) <= 0.f, label + " afterburner has no energy to consume");
		}
	}

	void RunModuleApplierPlayerChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		const std::array<const char*, 6> shipIds{
			"Ship.Player.Fighter.Basic", "Ship.Player.Breacher.Basic", "Ship.Player.Interceptor.Basic",
			"Ship.Player.Conductor.Basic", "Ship.Player.Aegis.Basic", "Ship.Player.CryoController.Basic"
		};
		const char* shieldModule = "Module.Defense.ShieldGenerator.Standard";
		const char* afterburnerModule = "Module.Movement.Afterburner.Standard";
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();

		for (const char* shipId : shipIds)
		{
			const std::string label = std::string{ shipId } + " modules:";
			manager.SetSelectedShipId(shipId);
			Player& player = manager.CreateNewPlayer();
			const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition(shipId);
			report.Check(player.GetModuleLoadout().GetPlacements().empty(), label + " loadout starts empty before first spawn");
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			level->TickInternal(0.f);
			report.Check(player.GetModuleLoadout().GetPlacements().size() == definition.schematic->defaultModules.size(),
				label + " first spawn seeds the schematic default modules");
			report.Check(ship->GetModuleApplier().HasApplied() && ship->GetEnabledModuleSystems().size() == 4,
				label + " default modules enable four systems");
			report.Check(ship->GetModuleApplier().GetOwnedHandleCount() == 0, label + " default modules own no handles");
			CheckEnergySystems(report, *ship, definition, 1, true, true, label + " default loadout level 1");

			player.AwardShipXP(definition.progressionDefinition.baseXP);
			level->TickInternal(0.f);
			CheckEnergySystems(report, *ship, definition, 2, true, true, label + " default loadout level 2");

			const ModulePlacement shield = *FindInstalled(player, shieldModule);
			const ModulePlacement afterburner = *FindInstalled(player, afterburnerModule);
			const std::uint64_t revisionBefore = player.GetModuleLoadout().GetRevision();
			report.Check(player.TryUninstallModule(shield.instanceId), label + " shield generator uninstalls");
			report.Check(player.GetModuleLoadout().GetRevision() > revisionBefore, label + " uninstall bumps revision");
			CheckEnergySystems(report, *ship, definition, 2, false, true, label + " shield removed");
			report.Check(player.TryInstallModule(shield.instanceId, shield.origin, shield.rotationQuarterTurns) == ModulePlacementFailure::None,
				label + " shield generator reinstalls");
			CheckEnergySystems(report, *ship, definition, 2, true, true, label + " shield restored with level growth");

			report.Check(player.TryUninstallModule(afterburner.instanceId), label + " afterburner uninstalls");
			CheckEnergySystems(report, *ship, definition, 2, true, false, label + " afterburner removed");

			ship->Destroy();
			level->TickInternal(0.f);
			ship = player.SpawnSpaceShip(level.get()).lock();
			level->TickInternal(0.f);
			report.Check(static_cast<bool>(ship) && ship->GetModuleApplier().HasApplied(), label + " respawned ship gets the loadout applied");
			CheckEnergySystems(report, *ship, definition, 2, true, false, label + " respawn keeps the removed afterburner");
			report.Check(ship->GetModuleApplier().GetAppliedRevision() == player.GetModuleLoadout().GetRevision(),
				label + " respawned ship tracks the loadout revision");
			report.Check(player.TryInstallModule(afterburner.instanceId, afterburner.origin, afterburner.rotationQuarterTurns) == ModulePlacementFailure::None,
				label + " afterburner reinstalls on the new ship");
			CheckEnergySystems(report, *ship, definition, 2, true, true, label + " all systems restored after respawn");

			ship->Destroy();
			level->TickInternal(0.f);
		}
	}

	const vega::ModulePlacement* FindInInventory(const vega::Player& player, const char* moduleId)
	{
		for (const vega::ModulePlacement& item : player.GetModuleLoadout().GetInventory())
		{
			if (item.moduleId == moduleId) return &item;
		}
		return nullptr;
	}

	std::set<std::uint32_t> InventoryInstanceIds(const vega::Player& player)
	{
		std::set<std::uint32_t> ids;
		for (const vega::ModulePlacement& item : player.GetModuleLoadout().GetInventory()) ids.insert(item.instanceId);
		return ids;
	}

	// kGrantAllModulesForTesting in Player.cpp: one instance of every size variant of every non-default module,
	// granted once per run, never installed automatically, kept across respawns.
	void RunModuleTestGrantChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();

		std::size_t standardCount = 0;
		std::set<std::string> grantable;
		std::set<std::pair<std::string, int>> grantableVariants;
		for (const ModuleDefinition& definition : content::ModuleContentCatalog::GetDefinitions())
		{
			const std::string id = definition.moduleId.ToString();
			if (id.size() >= 9 && id.compare(id.size() - 9, 9, ".Standard") == 0) { ++standardCount; continue; }
			grantable.insert(id);
			for (std::size_t variant = 0; variant < definition.sizeVariants.size(); ++variant) grantableVariants.insert({ id, static_cast<int>(variant) });
		}
		report.Check(grantableVariants.size() > grantable.size(), "test grant: some modules have more than one size variant");
		report.Check(standardCount == 11 && grantable.size() == 58, "test grant: shipped catalog has eleven standard and fifty-eight content modules");

		const std::array<const char*, 6> shipIds{
			"Ship.Player.Fighter.Basic", "Ship.Player.Breacher.Basic", "Ship.Player.Interceptor.Basic",
			"Ship.Player.Conductor.Basic", "Ship.Player.Aegis.Basic", "Ship.Player.CryoController.Basic"
		};
		for (const char* shipId : shipIds)
		{
			const std::string label = std::string{ shipId } + " test grant:";
			manager.SetSelectedShipId(shipId);
			Player& player = manager.CreateNewPlayer();
			const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition(shipId);
			report.Check(player.GetModuleLoadout().GetInventory().empty(), label + " inventory is empty before the first spawn");
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			level->TickInternal(0.f);

			const std::size_t defaultCount = definition.schematic->defaultModules.size();
			report.Check(player.GetModuleLoadout().GetInventory().size() == grantableVariants.size(), label + " inventory holds every size variant of every non-default module",
				{ { "actual", player.GetModuleLoadout().GetInventory().size() } });
			std::set<std::pair<std::string, int>> granted;
			for (const ModulePlacement& item : player.GetModuleLoadout().GetInventory())
			{
				report.Check(granted.insert({ item.moduleId.ToString(), item.variantIndex }).second,
					label + " grants " + item.moduleId.ToString() + " v" + std::to_string(item.variantIndex) + " only once");
			}
			report.Check(granted == grantableVariants, label + " grants each module variant exactly once");
			report.Check(player.GetModuleLoadout().GetPlacements().size() == defaultCount, label + " installs nothing beyond the defaults");
			CheckEnergySystems(report, *ship, definition, 1, true, true, label + " granted inventory does not change the ship");

			const std::set<std::uint32_t> idsBefore = InventoryInstanceIds(player);
			ship->Destroy();
			level->TickInternal(0.f);
			ship = player.SpawnSpaceShip(level.get()).lock();
			level->TickInternal(0.f);
			report.Check(static_cast<bool>(ship) && InventoryInstanceIds(player) == idsBefore, label + " respawn does not grant again");
			ship->Destroy();
			level->TickInternal(0.f);
		}

		// Fighter: installs from the granted inventory resolve to the intended numbers and survive respawn.
		manager.SetSelectedShipId("Ship.Player.Fighter.Basic");
		Player& player = manager.CreateNewPlayer();
		const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
		level->TickInternal(0.f);
		const std::size_t defaultCount = definition.schematic->defaultModules.size();
		const auto armor = [&ship]() { return ReadAttribute(ship->GetAbilitySystemComponent().GetAttributes(), OwnerAttributeIds::Armor); };
		const float baseArmor = armor();
		const float baseShield = definition.energyAttributes.baseMaxShield;
		const float baseShieldRegen = baseShield / definition.energyAttributes.shieldFullRechargeDuration;
		const float baseCapacity = definition.energyAttributes.baseAfterburnerCapacity;
		const float baseCapacityRegen = baseCapacity / definition.energyAttributes.afterburnerFullRechargeDuration;

		const ModulePlacement* armorItem = FindInInventory(player, "Module.Defense.ArmorPlate.Basic");
		report.Check(armorItem != nullptr, "test grant: Fighter has an Armor Plate in the inventory");
		report.Check(player.TryInstallModule(armorItem->instanceId, sf::Vector2i{ 2, 3 }, 0) == ModulePlacementFailure::None,
			"test grant: Armor Plate installs into the Fighter Core");
		report.Check(NearlyEqual(armor(), baseArmor + 5.f), "test grant: Armor Plate in the matching zone adds +5 armor",
			{ { "actual", armor() }, { "expected", baseArmor + 5.f } });

		ship->Destroy();
		level->TickInternal(0.f);
		ship = player.SpawnSpaceShip(level.get()).lock();
		level->TickInternal(0.f);
		report.Check(player.GetModuleLoadout().GetInventory().size() == grantableVariants.size() - 1 &&
				player.GetModuleLoadout().GetPlacements().size() == defaultCount + 1,
			"test grant: respawn keeps the installed module and does not regrant it");
		report.Check(NearlyEqual(armor(), baseArmor + 5.f), "test grant: Armor Plate survives respawn");

		const auto swapShield = [&](const char* moduleId, sf::Vector2i origin, int rotation = 0)
		{
			const ModulePlacement* installed = FindInstalled(player, "Module.Defense.ShieldGenerator.Standard");
			if (!installed) installed = FindInstalled(player, "Module.Defense.ShieldGenerator.FastCharge");
			if (!installed) installed = FindInstalled(player, "Module.Defense.ShieldGenerator.Heavy");
			report.Check(installed != nullptr && player.TryUninstallModule(installed->instanceId), "test grant: current shield generator uninstalls");
			const ModulePlacement* item = FindInInventory(player, moduleId);
			report.Check(item != nullptr, std::string("test grant: inventory holds ") + moduleId);
			report.Check(player.TryInstallModule(item->instanceId, origin, rotation) == ModulePlacementFailure::None, std::string("test grant: installs ") + moduleId);
		};
		const sas::AttributeSystem& liveShipAttributes = ship->GetShipRuntime().GetAttributes();
		swapShield("Module.Defense.ShieldGenerator.FastCharge", sf::Vector2i{ 1, 5 });
		const float fastMax = ReadAttribute(liveShipAttributes, ShipAttributeIds::MaxShield);
		const float fastRegen = ReadAttribute(liveShipAttributes, ShipAttributeIds::ShieldRegen);
		report.Check(NearlyEqual(fastMax, baseShield * 0.8f), "test grant: Fast Charge max shield is 80% (zone bonus does not scale the penalty)",
			{ { "actual", fastMax }, { "expected", baseShield * 0.8f } });
		report.Check(NearlyEqual(fastRegen, baseShieldRegen * 1.625f), "test grant: Fast Charge regen is x1.625 of the unmodified rate",
			{ { "actual", fastRegen }, { "expected", baseShieldRegen * 1.625f } });
		report.Check(fastMax / fastRegen < baseShield / baseShieldRegen * 0.6f, "test grant: Fast Charge recharges a full shield in about half the time",
			{ { "actualSeconds", fastMax / fastRegen }, { "standardSeconds", baseShield / baseShieldRegen } });

		swapShield("Module.Defense.ShieldGenerator.Heavy", sf::Vector2i{ 5, 3 }, 1);
		report.Check(NearlyEqual(ReadAttribute(liveShipAttributes, ShipAttributeIds::MaxShield), baseShield + 75.f), "test grant: Heavy shield adds +75 in the Core",
			{ { "actual", ReadAttribute(liveShipAttributes, ShipAttributeIds::MaxShield) }, { "expected", baseShield + 75.f } });
		report.Check(NearlyEqual(ReadAttribute(liveShipAttributes, ShipAttributeIds::ShieldRegen), baseShieldRegen), "test grant: Heavy shield keeps the base regen rate");

		const ModulePlacement* afterburner = FindInstalled(player, "Module.Movement.Afterburner.Standard");
		report.Check(afterburner != nullptr && player.TryUninstallModule(afterburner->instanceId), "test grant: standard afterburner uninstalls");
		const ModulePlacement* extended = FindInInventory(player, "Module.Movement.Afterburner.Extended");
		report.Check(extended != nullptr && player.TryInstallModule(extended->instanceId, sf::Vector2i{ 4, 6 }, 0) == ModulePlacementFailure::None,
			"test grant: Extended afterburner installs into the Engine");
		report.Check(NearlyEqual(ReadAttribute(liveShipAttributes, ShipAttributeIds::AfterburnerCapacity), baseCapacity + 62.5f),
			"test grant: Extended afterburner adds +62.5 capacity in the Engine",
			{ { "actual", ReadAttribute(liveShipAttributes, ShipAttributeIds::AfterburnerCapacity) }, { "expected", baseCapacity + 62.5f } });
		report.Check(NearlyEqual(ReadAttribute(liveShipAttributes, ShipAttributeIds::AfterburnerRegen), baseCapacityRegen),
			"test grant: Extended afterburner keeps the base regen rate");

		ship->Destroy();
		level->TickInternal(0.f);
	}

	void RunModuleApplierHandleChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		namespace Tags = GameplayTags;
		const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();
		shared_ptr<PlayerSpaceShip> ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
		report.Check(static_cast<bool>(ship), "applier test spawns a standalone player ship");
		level->TickInternal(0.f);

		ModuleDefinition test;
		test.moduleId = "Module.Offense.TestEmitter.Standard";
		test.family = Tags::Ability::Offense;
		ModuleSizeVariant variant;
		variant.shape.cells = { { 0, 0 } };
		ModuleAttributeModifier ownerModifier;
		ownerModifier.target = ModuleAttributeTarget::Owner;
		ownerModifier.modifier = sas::AttributeModifier{ OwnerAttributeIds::Armor, 5.f };
		ownerModifier.affectedByZoneBonus = false;
		ModuleAttributeModifier shipModifier;
		shipModifier.target = ModuleAttributeTarget::Ship;
		shipModifier.modifier = sas::AttributeModifier{ ShipAttributeIds::AfterburnerEnergyDrainPerSecond, 3.f };
		shipModifier.affectedByZoneBonus = false;
		variant.attributeModifiers = { ownerModifier, shipModifier };
		const sas::AttributeId abilityAttribute{ "Ability.Test.Damage" };
		variant.abilityModifiers = { ModuleAbilityModifier{ sas::AttributeModifier{ abilityAttribute, 7.f }, false } };
		test.sizeVariants.push_back(variant);

		const ModuleRulesContext rules{
			*definition.schematic,
			[&test](const sas::ContentId& id) -> const ModuleDefinition*
			{
				return id == test.moduleId ? &test : content::ModuleContentCatalog::FindById(id);
			},
			content::ModuleContentCatalog::GetZoneTable()
		};

		ModuleLoadout loadout;
		const std::uint32_t testId = loadout.AddToInventory(test.moduleId);
		ModulePlacement probe;
		probe.moduleId = test.moduleId;
		sf::Vector2i freeCell{ -1, -1 };
		for (int y = 0; y < definition.schematic->height && freeCell.x < 0; ++y)
		{
			for (int x = 0; x < definition.schematic->width; ++x)
			{
				probe.origin = { x, y };
				if (ModulePlacementRules::CanPlace(rules, loadout, probe) == ModulePlacementFailure::None) { freeCell = probe.origin; break; }
			}
		}
		report.Check(freeCell.x >= 0, "applier test finds a free schematic cell");

		PlayerSpaceShip& target = *ship;
		ShipModuleApplier& applier = target.GetModuleApplier();
		sas::AttributeSystem& owner = target.GetAbilitySystemComponent().GetAttributes();
		const sas::AttributeSystem& shipAttributes = target.GetShipRuntime().GetAttributes();
		GameAbilityDefinition abilityDefinition;
		abilityDefinition.abilityTags.push_back(Tags::Ability::Offense);
		auto scopedValue = [&]()
		{
			return target.GetAbilitySystemComponent().ApplyScopedAbilityModifiers(
				abilityDefinition, sas::GameplayAttribute{ abilityAttribute, 10.f }).currentValue;
		};
		const float armorBase = ReadAttribute(owner, OwnerAttributeIds::Armor);
		const float drainBase = ReadAttribute(shipAttributes, ShipAttributeIds::AfterburnerEnergyDrainPerSecond);

		report.Check(applier.Apply(loadout, rules) && applier.GetOwnedHandleCount() == 0, "empty loadout applies with no handles");
		report.Check(!applier.Apply(loadout, rules), "unchanged revision does not reapply");
		report.Check(!target.GetShipRuntime().IsSystemEnabled(ShipSystem::Shield) &&
			!target.GetShipRuntime().IsSystemEnabled(ShipSystem::Afterburner), "empty loadout disables both systems");

		for (int cycle = 0; cycle < 6; ++cycle)
		{
			const std::string label = "applier test cycle " + std::to_string(cycle);
			report.Check(loadout.Install(rules, testId, freeCell, 0) == ModulePlacementFailure::None, label + " installs the test module");
			report.Check(applier.Apply(loadout, rules), label + " reapplies after install");
			report.Check(NearlyEqual(ReadAttribute(owner, OwnerAttributeIds::Armor), armorBase + 5.f), label + " owner modifier applied",
				ReadAttribute(owner, OwnerAttributeIds::Armor));
			report.Check(NearlyEqual(ReadAttribute(shipAttributes, ShipAttributeIds::AfterburnerEnergyDrainPerSecond), drainBase + 3.f),
				label + " ship modifier applied");
			report.Check(NearlyEqual(scopedValue(), 17.f), label + " scoped ability value applied", scopedValue());
			report.Check(applier.GetOwnedHandleCount() == 3, label + " owns exactly three handles", applier.GetOwnedHandleCount());

			report.Check(loadout.Uninstall(testId), label + " uninstalls the test module");
			report.Check(applier.Apply(loadout, rules), label + " reapplies after uninstall");
			report.Check(NearlyEqual(ReadAttribute(owner, OwnerAttributeIds::Armor), armorBase), label + " owner modifier restored");
			report.Check(NearlyEqual(ReadAttribute(shipAttributes, ShipAttributeIds::AfterburnerEnergyDrainPerSecond), drainBase),
				label + " ship modifier restored");
			report.Check(NearlyEqual(scopedValue(), 10.f), label + " scoped ability value restored");
			report.Check(applier.GetOwnedHandleCount() == 0, label + " leaves no handles");
		}
		ship->Destroy();
		level->TickInternal(0.f);
	}

	void RunModuleAbilityAndPrimaryRuleChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		namespace Tags = GameplayTags;
		const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();
		shared_ptr<PlayerSpaceShip> ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
		report.Check(static_cast<bool>(ship), "ability rule test spawns a player ship");
		level->TickInternal(0.f);

		auto makeModule = [](const char* id, const GameplayTag& family, bool primaryTarget)
		{
			ModuleDefinition module;
			module.moduleId = id;
			module.family = family;
			ModuleSizeVariant variant;
			variant.shape.cells = { { 0, 0 } };
			if (primaryTarget)
			{
				ModuleAttributeModifier damage;
				damage.target = ModuleAttributeTarget::PrimaryWeapon;
				damage.modifier = sas::AttributeModifier{ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Multiply, 1.5f };
				ModuleAttributeModifier fireRate;
				fireRate.target = ModuleAttributeTarget::PrimaryWeapon;
				fireRate.modifier = sas::AttributeModifier{ CommonAttributeIds::FireRate, sas::AttributeModifierOperation::Multiply, 1.2f };
				variant.attributeModifiers = { damage, fireRate };
			}
			else
			{
				variant.abilityModifiers = { ModuleAbilityModifier{
					sas::AttributeModifier{ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Multiply, 1.5f }, true } };
			}
			module.sizeVariants.push_back(variant);
			return module;
		};
		const ModuleDefinition offenseModule = makeModule("Module.Offense.TestDamage.Standard", Tags::Ability::Offense, false);
		const ModuleDefinition primaryModule = makeModule("Module.Primary.TestWeapon.Standard", Tags::Ability::Primary, true);
		const ModuleRulesContext rules{
			*definition.schematic,
			[&](const sas::ContentId& id) -> const ModuleDefinition*
			{
				if (id == offenseModule.moduleId) return &offenseModule;
				if (id == primaryModule.moduleId) return &primaryModule;
				return content::ModuleContentCatalog::FindById(id);
			},
			content::ModuleContentCatalog::GetZoneTable()
		};

		ModuleLoadout loadout;
		const std::uint32_t offenseId = loadout.AddToInventory(offenseModule.moduleId);
		const std::uint32_t primaryId = loadout.AddToInventory(primaryModule.moduleId);
		auto bestCell = [&](const ModuleDefinition& module, std::uint32_t instanceId)
		{
			ModulePlacement probe;
			probe.moduleId = module.moduleId;
			sf::Vector2i best{ -1, -1 };
			float bestBonus = -1.f;
			for (int y = 0; y < definition.schematic->height; ++y)
			{
				for (int x = 0; x < definition.schematic->width; ++x)
				{
					probe.origin = { x, y };
					if (ModulePlacementRules::CanPlace(rules, loadout, probe) != ModulePlacementFailure::None) continue;
					const float bonus = ModuleResolver::GetZoneBonus(rules, module, probe);
					if (bonus > bestBonus) { bestBonus = bonus; best = probe.origin; }
				}
			}
			(void)instanceId;
			return std::make_pair(best, bestBonus);
		};
		const auto offenseCell = bestCell(offenseModule, offenseId);
		const auto primaryCell = bestCell(primaryModule, primaryId);
		report.Check(offenseCell.first.x >= 0 && primaryCell.first.x >= 0, "ability rule test finds free cells");
		const float offenseBonus = offenseCell.second;
		const float primaryBonus = primaryCell.second;
		report.Check(offenseBonus > 0.f && primaryBonus > 0.f, "ability rule test picks zone-bonus cells", offenseBonus);
		const float damageMultiplier = 1.f + 0.5f * (1.f + offenseBonus);
		const float primaryDamageMultiplier = 1.f + 0.5f * (1.f + primaryBonus);
		const float primaryFireRateMultiplier = 1.f + 0.2f * (1.f + primaryBonus);

		PlayerSpaceShip& target = *ship;
		ShipModuleApplier& applier = target.GetModuleApplier();
		LightYearsAbilitySystemComponent& abilities = target.GetAbilitySystemComponent();
		GameAbility* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		const FireWeaponAction* fireAction = nullptr;
		if (primary)
		{
			for (const AbilityActionSpec& action : primary->GetDefinition().actions)
			{
				if ((fireAction = std::get_if<FireWeaponAction>(&action.action))) break;
			}
		}
		report.Check(primary && fireAction, "ability rule test finds the primary fire action");
		if (!primary || !fireAction) { ship->Destroy(); level->TickInternal(0.f); return; }

		// A scoped refresh rebuilds the ability definition, so keep a copy rather than a pointer into it.
		const PrimaryWeaponDefinition weaponCopy = fireAction->weaponDefinition;
		GameAbilityDefinition offenseAbility;
		offenseAbility.abilityTags.push_back(Tags::Ability::Offense);
		GameAbilityDefinition utilityAbility;
		utilityAbility.abilityTags.push_back(Tags::Ability::Utility);
		GameAbilityDefinition primaryLikeAbility = primary->GetDefinition();
		const sas::GameplayAttributeList probeAttributes{
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f, 10000.f },
			sas::GameplayAttribute{ CommonAttributeIds::FireRate, 2.f, 0.f, 10000.f } };
		auto resolve = [&](GameAbilityDefinition& ability, const PrimaryWeaponDefinition* weapon, const char* attribute)
		{
			AbilityExecutionContext context{ &abilities, &ability, nullptr, nullptr };
			return sas::FindAttributeValue(AbilityActionAttributeResolver::ResolveAttributes(context, weapon, probeAttributes),
				sas::AttributeId{ attribute }, -1.f);
		};
		const char* damageId = "Common.Damage";
		const char* fireRateId = "Common.FireRate";
		applier.Apply(loadout, rules);
		// Module weapon modifiers act on the authored value before owner scaling (like weapon attributeModifiers),
		// so the additive scaling share (base - 10) stays unscaled.
		const float primaryDamageBase = resolve(primaryLikeAbility, &weaponCopy, damageId);
		GameAbilityDefinition offenseWithWeaponProbe = offenseAbility;
		const float nonPrimaryWeaponBase = resolve(offenseWithWeaponProbe, &weaponCopy, damageId);
		const float primaryFireRateBase = resolve(primaryLikeAbility, &weaponCopy, fireRateId);

		const std::uint64_t configBefore = primary->GetConfigurationRevision();
		for (int cycle = 0; cycle < 5; ++cycle)
		{
			const std::string label = "ability rule cycle " + std::to_string(cycle);
			const std::uint64_t modifierRevision = abilities.GetPrimaryWeaponModifierRevision();

			// (a) Offense family scoped rule.
			report.Check(loadout.Install(rules, offenseId, offenseCell.first, 0) == ModulePlacementFailure::None, label + " installs offense module");
			report.Check(applier.Apply(loadout, rules), label + " applies offense module");
			report.Check(NearlyEqual(resolve(offenseAbility, nullptr, damageId), 10.f * damageMultiplier), label + " offense ability damage scaled",
				resolve(offenseAbility, nullptr, damageId));
			report.Check(NearlyEqual(resolve(utilityAbility, nullptr, damageId), 10.f), label + " utility ability unaffected");
			report.Check(NearlyEqual(resolve(primaryLikeAbility, &weaponCopy, damageId), primaryDamageBase),
				label + " scoped offense rule skips the primary weapon", { { "base", primaryDamageBase },
					{ "now", resolve(primaryLikeAbility, &weaponCopy, damageId) } });
			report.Check(primary->GetConfigurationRevision() > configBefore, label + " scoped rule invalidates ability configuration cache");
			report.Check(abilities.GetPrimaryWeaponModifierRevision() == modifierRevision, label + " offense module leaves primary modifier revision");
			report.Check(loadout.Uninstall(offenseId) && applier.Apply(loadout, rules), label + " uninstalls offense module");
			report.Check(NearlyEqual(resolve(offenseAbility, nullptr, damageId), 10.f), label + " offense ability damage restored");

			// (b) Primary family weapon modifiers.
			report.Check(loadout.Install(rules, primaryId, primaryCell.first, 0) == ModulePlacementFailure::None, label + " installs primary module");
			report.Check(applier.Apply(loadout, rules), label + " applies primary module");
			report.Check(abilities.GetPrimaryWeaponModifierRevision() > modifierRevision, label + " primary module bumps modifier revision");
			report.Check(NearlyEqual(resolve(primaryLikeAbility, &weaponCopy, damageId), 10.f * primaryDamageMultiplier + (primaryDamageBase - 10.f)),
				label + " primary weapon damage modified without leveling", resolve(primaryLikeAbility, &weaponCopy, damageId));
			report.Check(NearlyEqual(resolve(primaryLikeAbility, &weaponCopy, fireRateId), primaryFireRateBase * primaryFireRateMultiplier),
				label + " primary weapon fire rate modified", resolve(primaryLikeAbility, &weaponCopy, fireRateId));
			report.Check(NearlyEqual(resolve(offenseAbility, nullptr, damageId), 10.f) && NearlyEqual(resolve(utilityAbility, nullptr, damageId), 10.f),
				label + " primary module does not touch non-primary abilities");
			GameAbilityDefinition offenseWithWeapon = offenseAbility;
			report.Check(NearlyEqual(resolve(offenseWithWeapon, &weaponCopy, damageId), nonPrimaryWeaponBase),
				label + " non-primary-slot weapon branch is untouched");
			const std::uint64_t installedRevision = abilities.GetPrimaryWeaponModifierRevision();
			report.Check(!applier.Apply(loadout, rules) && abilities.GetPrimaryWeaponModifierRevision() == installedRevision,
				label + " no-op apply keeps modifier revision");
			report.Check(loadout.Uninstall(primaryId) && applier.Apply(loadout, rules), label + " uninstalls primary module");
			report.Check(abilities.GetPrimaryWeaponModifierRevision() > installedRevision, label + " uninstall bumps modifier revision");
			report.Check(NearlyEqual(resolve(primaryLikeAbility, &weaponCopy, damageId), primaryDamageBase),
				label + " primary weapon damage restored");
			report.Check(applier.GetOwnedHandleCount() == 0, label + " leaves no handles", applier.GetOwnedHandleCount());
		}
		ship->Destroy();
		level->TickInternal(0.f);
	}

	void RunModuleRebootAndClampChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
		namespace Tags = GameplayTags;
		const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
		auto level = std::make_shared<HeadlessShipLevel>(&application);
		level->BeginPlayInternal();
		shared_ptr<PlayerSpaceShip> ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
		shared_ptr<PlayerSpaceShip> dummy = level->SpawnActor<PlayerSpaceShip>(definition).lock();
		level->TickInternal(0.f);
		ship->SetInvulnerability(false);
		dummy->SetInvulnerability(false);

		auto makeModule = [](const char* id, std::initializer_list<std::pair<ModuleAttributeTarget, sas::AttributeModifier>> modifiers)
		{
			ModuleDefinition module;
			module.moduleId = id;
			module.family = Tags::Ability::Offense;
			ModuleSizeVariant variant;
			variant.shape.cells = { { 0, 0 } };
			for (const auto& [target, modifier] : modifiers)
			{
				ModuleAttributeModifier entry;
				entry.target = target;
				entry.modifier = modifier;
				entry.affectedByZoneBonus = false;
				variant.attributeModifiers.push_back(entry);
			}
			module.sizeVariants.push_back(variant);
			return module;
		};
		const ModuleDefinition armorModule = makeModule("Module.Offense.TestArmor.Standard",
			{ { ModuleAttributeTarget::Owner, sas::AttributeModifier{ OwnerAttributeIds::Armor, 5.f } } });
		const ModuleDefinition energyModule = makeModule("Module.Offense.TestEnergy.Standard", {
			{ ModuleAttributeTarget::Ship, sas::AttributeModifier{ ShipAttributeIds::MaxShield, 20.f } },
			{ ModuleAttributeTarget::Ship, sas::AttributeModifier{ ShipAttributeIds::ShieldRegen, 2.f } },
			{ ModuleAttributeTarget::Ship, sas::AttributeModifier{ ShipAttributeIds::AfterburnerCapacity, 10.f } },
			{ ModuleAttributeTarget::Ship, sas::AttributeModifier{ ShipAttributeIds::AfterburnerRegen, 1.f } } });
		const ModuleRulesContext rules{
			*definition.schematic,
			[&](const sas::ContentId& id) -> const ModuleDefinition*
			{
				if (id == armorModule.moduleId) return &armorModule;
				if (id == energyModule.moduleId) return &energyModule;
				return content::ModuleContentCatalog::FindById(id);
			},
			content::ModuleContentCatalog::GetZoneTable()
		};
		const float rebootSeconds = rules.zoneTable.rebootDurationSeconds;
		const float combatWindow = rules.zoneTable.combatWindowSeconds;
		report.Check(NearlyEqual(rebootSeconds, 1.5f) && NearlyEqual(combatWindow, 3.f), "reboot test reads 1.5 s reboot and 3 s window");

		ModuleLoadout loadout;
		const std::uint32_t armorId = loadout.AddToInventory(armorModule.moduleId);
		std::vector<sf::Vector2i> freeCells;
		for (int y = 0; y < definition.schematic->height; ++y)
		{
			for (int x = 0; x < definition.schematic->width; ++x)
			{
				ModulePlacement probe;
				probe.moduleId = armorModule.moduleId;
				probe.origin = { x, y };
				if (ModulePlacementRules::CanPlace(rules, loadout, probe) == ModulePlacementFailure::None) freeCells.push_back({ x, y });
			}
		}
		report.Check(freeCells.size() >= 3, "reboot test finds free schematic cells");

		ShipModuleApplier& applier = ship->GetModuleApplier();
		sas::AttributeSystem& owner = ship->GetAbilitySystemComponent().GetAttributes();
		const float armorBase = ReadAttribute(owner, OwnerAttributeIds::Armor);
		auto armorActive = [&]() { return NearlyEqual(ReadAttribute(owner, OwnerAttributeIds::Armor), armorBase + 5.f); };
		auto apply = [&]() { return applier.Apply(loadout, rules, ship->IsInCombat()); };
		auto damageShip = [&]() { ApplyCombatDamage(*ship, 1.f, dummy.get()); };

		report.Check(apply(), "reboot test first apply");
		report.Check(!ship->IsInCombat(), "fresh ship is not in combat");

		// Out of combat: immediate in both directions.
		report.Check(loadout.Install(rules, armorId, freeCells[0], 0) == ModulePlacementFailure::None, "out-of-combat install");
		apply();
		report.Check(armorActive() && !applier.IsRebooting(armorId), "out of combat install is immediate");
		loadout.Uninstall(armorId);
		apply();
		report.Check(!armorActive(), "out of combat uninstall is immediate");

		// In combat: reboot for 1.5 s.
		damageShip();
		report.Check(ship->IsInCombat(), "taking damage starts combat");
		loadout.Install(rules, armorId, freeCells[0], 0);
		apply();
		report.Check(!armorActive() && applier.IsRebooting(armorId), "in-combat install enters reboot");
		report.Check(NearlyEqual(applier.GetRebootRemainingSeconds(armorId), rebootSeconds), "reboot remaining starts at 1.5 s");
		level->TickInternal(1.4f);
		report.Check(!armorActive() && applier.IsRebooting(armorId), "module inactive at 1.4 s");
		report.Check(NearlyEqual(applier.GetRebootRemainingSeconds(armorId), 0.1f), "reboot remaining is 0.1 s at 1.4 s",
			applier.GetRebootRemainingSeconds(armorId));
		level->TickInternal(0.11f);
		report.Check(armorActive() && !applier.IsRebooting(armorId), "module active after 1.5 s");

		// Moving an installed module is not a reboot, even in combat.
		report.Check(ship->IsInCombat(), "still in combat after 1.51 s");
		report.Check(loadout.Move(rules, armorId, freeCells[1], 0) == ModulePlacementFailure::None, "move installed module");
		apply();
		report.Check(armorActive() && !applier.IsRebooting(armorId), "moving a module in combat does not reboot it");

		// Uninstall in combat is immediate; reinstall reboots again.
		loadout.Uninstall(armorId);
		apply();
		report.Check(!armorActive(), "uninstall during combat is immediate");
		loadout.Install(rules, armorId, freeCells[2], 0);
		apply();
		report.Check(!armorActive() && applier.IsRebooting(armorId), "reinstall during combat reboots again");
		// An unrelated revision change must not restart or finish the countdown.
		level->TickInternal(0.5f);
		const float remainingBefore = applier.GetRebootRemainingSeconds(armorId);
		loadout.Move(rules, armorId, freeCells[0], 0);
		apply();
		report.Check(NearlyEqual(applier.GetRebootRemainingSeconds(armorId), remainingBefore), "revision change keeps the running countdown");
		loadout.Uninstall(armorId);
		apply();
		report.Check(!applier.IsRebooting(armorId), "uninstalling a rebooting module cancels the reboot");

		// Combat window: 3 s after the last damage, damage dealt counts too.
		level->TickInternal(combatWindow + 0.1f);
		report.Check(!ship->IsInCombat(), "combat window expires");
		damageShip();
		level->TickInternal(2.9f);
		report.Check(ship->IsInCombat(), "still in combat 2.9 s after damage");
		level->TickInternal(0.2f);
		report.Check(!ship->IsInCombat(), "combat ends 3 s after the last damage");
		ApplyCombatDamage(*dummy, 1.f, ship.get());
		report.Check(ship->IsInCombat(), "dealing damage starts combat");
		level->TickInternal(combatWindow + 0.1f);
		report.Check(!ship->IsInCombat(), "dealt-damage combat window expires");
		loadout.Install(rules, armorId, freeCells[0], 0);
		apply();
		report.Check(armorActive(), "install after the window is immediate");
		loadout.Uninstall(armorId);
		apply();

		// First apply on a new ship is immediate even in combat.
		ModuleLoadout spawnLoadout;
		const std::uint32_t spawnArmor = spawnLoadout.AddToInventory(armorModule.moduleId);
		spawnLoadout.Install(rules, spawnArmor, freeCells[0], 0);
		ShipModuleApplier& freshApplier = dummy->GetModuleApplier();
		report.Check(!freshApplier.HasApplied(), "dummy ship has not applied yet");
		freshApplier.Apply(spawnLoadout, rules, true);
		sas::AttributeSystem& dummyOwner = dummy->GetAbilitySystemComponent().GetAttributes();
		report.Check(!freshApplier.IsRebooting(spawnArmor) &&
			NearlyEqual(ReadAttribute(dummyOwner, OwnerAttributeIds::Armor), armorBase + 5.f), "first apply is immediate even in combat");

		// Fix A: removed systems cannot be powered by other modules.
		ModuleLoadout energyLoadout;
		report.Check(energyLoadout.SeedDefaults(rules), "default loadout seeds for the clamp test");
		const std::uint32_t energyId = energyLoadout.AddToInventory(energyModule.moduleId);
		report.Check(energyLoadout.Install(rules, energyId, freeCells[0], 0) == ModulePlacementFailure::None, "energy test module installs");
		const sas::AttributeSystem& shipAttributes = ship->GetShipRuntime().GetAttributes();
		auto read = [&](const sas::AttributeId& id) { return ReadAttribute(shipAttributes, id); };
		const ShipEnergyAttributes& energy = definition.energyAttributes;
		const float baseShield = energy.baseMaxShield;
		const float baseCapacity = energy.baseAfterburnerCapacity;
		const float shieldRegen = baseShield / std::max(0.001f, energy.shieldFullRechargeDuration);
		const float afterburnerRegen = baseCapacity / std::max(0.001f, energy.afterburnerFullRechargeDuration);
		// The ship is level 1, so no growth term applies.
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(read(ShipAttributeIds::MaxShield), baseShield + 20.f), "energy module adds MaxShield", read(ShipAttributeIds::MaxShield));
		report.Check(NearlyEqual(read(ShipAttributeIds::ShieldRegen), shieldRegen + 2.f), "energy module adds ShieldRegen");
		report.Check(NearlyEqual(read(ShipAttributeIds::AfterburnerCapacity), baseCapacity + 10.f), "energy module adds AfterburnerCapacity");
		report.Check(NearlyEqual(read(ShipAttributeIds::AfterburnerRegen), afterburnerRegen + 1.f), "energy module adds AfterburnerRegen");

		auto findInstalled = [&](const char* moduleId) -> ModulePlacement
		{
			for (const ModulePlacement& placement : energyLoadout.GetPlacements())
			{
				if (placement.moduleId == moduleId) return placement;
			}
			return {};
		};
		const ModulePlacement shieldGenerator = findInstalled("Module.Defense.ShieldGenerator.Standard");
		const ModulePlacement afterburnerModule = findInstalled("Module.Movement.Afterburner.Standard");
		energyLoadout.Uninstall(shieldGenerator.instanceId);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(read(ShipAttributeIds::MaxShield), 0.f) && NearlyEqual(read(ShipAttributeIds::ShieldRegen), 0.f),
			"modules cannot power a removed shield generator", read(ShipAttributeIds::MaxShield));
		report.Check(NearlyEqual(ship->GetShieldComponent().GetMaxShield(), 0.f) && NearlyEqual(ship->GetShieldComponent().GetShield(), 0.f),
			"removed shield leaves the live shield empty");
		report.Check(NearlyEqual(read(ShipAttributeIds::AfterburnerCapacity), baseCapacity + 10.f), "afterburner unaffected by shield removal");
		energyLoadout.Install(rules, shieldGenerator.instanceId, shieldGenerator.origin, shieldGenerator.rotationQuarterTurns);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(read(ShipAttributeIds::MaxShield), baseShield + 20.f) &&
			NearlyEqual(read(ShipAttributeIds::ShieldRegen), shieldRegen + 2.f), "reinstalled shield is base plus module",
			read(ShipAttributeIds::MaxShield));
		report.Check(NearlyEqual(ship->GetShieldComponent().GetMaxShield(), baseShield + 20.f), "live shield max follows reinstall");

		energyLoadout.Uninstall(afterburnerModule.instanceId);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(read(ShipAttributeIds::AfterburnerCapacity), 0.f) && NearlyEqual(read(ShipAttributeIds::AfterburnerRegen), 0.f),
			"modules cannot power a removed afterburner", read(ShipAttributeIds::AfterburnerCapacity));
		report.Check(NearlyEqual(ship->GetEnergyComponent().GetMaxEnergy(), 0.f), "removed afterburner has no live energy capacity");
		energyLoadout.Install(rules, afterburnerModule.instanceId, afterburnerModule.origin, afterburnerModule.rotationQuarterTurns);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(read(ShipAttributeIds::AfterburnerCapacity), baseCapacity + 10.f) &&
			NearlyEqual(read(ShipAttributeIds::AfterburnerRegen), afterburnerRegen + 1.f), "reinstalled afterburner is base plus module");

		// Fix B: reapply must not clip or refill current shield and afterburner energy.
		ShieldComponent& shield = ship->GetShieldComponent();
		EnergyComponent& energyComponent = ship->GetEnergyComponent();
		const float shieldMax = baseShield + 20.f;
		const float capacityMax = baseCapacity + 10.f;
		shield.ChangeShield(-shieldMax * 0.4f);
		energyComponent.Consume(capacityMax * 0.3f);
		const float shieldBefore = shield.GetShield();
		const float energyBefore = energyComponent.GetEnergy();
		report.Check(shieldBefore > 0.f && shieldBefore < shieldMax && energyBefore > 0.f && energyBefore < capacityMax,
			"partial shield and energy set up for the reapply test");
		const std::uint32_t armorTwo = energyLoadout.AddToInventory(armorModule.moduleId);
		report.Check(energyLoadout.Install(rules, armorTwo, freeCells[1], 0) == ModulePlacementFailure::None, "unrelated module installs");
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(shield.GetShield(), shieldBefore) && NearlyEqual(shield.GetMaxShield(), shieldMax),
			"unrelated reapply leaves current shield unchanged", shield.GetShield());
		report.Check(NearlyEqual(energyComponent.GetEnergy(), energyBefore) && NearlyEqual(energyComponent.GetMaxEnergy(), capacityMax),
			"unrelated reapply leaves afterburner energy unchanged", energyComponent.GetEnergy());
		energyLoadout.Uninstall(armorTwo);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(shield.GetShield(), shieldBefore) && NearlyEqual(energyComponent.GetEnergy(), energyBefore),
			"unrelated uninstall leaves shield and energy unchanged");

		// Lowering max only clamps; raising max never refills.
		energyLoadout.Uninstall(energyId);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(shield.GetMaxShield(), baseShield) &&
			NearlyEqual(shield.GetShield(), std::min(shieldBefore, baseShield)), "lowering max shield only clamps", shield.GetShield());
		report.Check(NearlyEqual(energyComponent.GetMaxEnergy(), baseCapacity) &&
			NearlyEqual(energyComponent.GetEnergy(), std::min(energyBefore, baseCapacity)), "lowering max energy only clamps", energyComponent.GetEnergy());
		const float shieldLowered = shield.GetShield();
		const float energyLowered = energyComponent.GetEnergy();
		energyLoadout.Install(rules, energyId, freeCells[0], 0);
		applier.Apply(energyLoadout, rules, false);
		report.Check(NearlyEqual(shield.GetShield(), shieldLowered) && NearlyEqual(shield.GetMaxShield(), shieldMax),
			"raising max shield does not refill", shield.GetShield());
		report.Check(NearlyEqual(energyComponent.GetEnergy(), energyLowered) && NearlyEqual(energyComponent.GetMaxEnergy(), capacityMax),
			"raising max energy does not refill", energyComponent.GetEnergy());

		dummy->Destroy();
		ship->Destroy();
		level->TickInternal(0.f);
	}

	void RunSelectionKeyChecks(Report& report, vega::Application& application)
	{
		using namespace vega;
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

		level->DispatchKeyPressOnly(sf::Keyboard::Key::F6);
		level->TickInternal(0.f);
		const shared_ptr<PlayerSpaceShip> heldShip =
			manager.GetPlayer() ? manager.GetPlayer()->GetCurrentSpaceShip().lock() : shared_ptr<PlayerSpaceShip>{};
		level->DispatchKeyPressOnly(sf::Keyboard::Key::F6);
		level->TickInternal(0.f);
		const shared_ptr<PlayerSpaceShip> afterRepeat =
			manager.GetPlayer() ? manager.GetPlayer()->GetCurrentSpaceShip().lock() : shared_ptr<PlayerSpaceShip>{};
		report.Check(heldShip && afterRepeat == heldShip && !heldShip->GetIsPendingDestroy(),
			"key repeat while a ship key is held does not restart again");
		level->ReleaseKey(sf::Keyboard::Key::F6);

		const shared_ptr<PlayerSpaceShip> beforePause = afterRepeat;
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

	void RunEnemyShieldRegression(Report& report, vega::Application& application)
	{
		using namespace vega;
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

		report.Check(enemy->GetShipRuntime().IsSystemEnabled(ShipSystem::Shield) &&
			enemy->GetShipRuntime().IsSystemEnabled(ShipSystem::Afterburner), "enemy ship systems default to enabled");
		enemy->GetShipRuntime().InitializeFromShipDefinition(*enemyShip);
		enemy->GetShipRuntime().SetShipLevel(3);
		enemy->GetShipRuntime().SetBaseMaxShieldContribution("Test.Source", 4.f);
		const float withContribution = ReadAttribute(attributes, ShipAttributeIds::MaxShield);
		enemy->GetShipRuntime().SetSystemEnabled(ShipSystem::Shield, false);
		const float disabledShield = ReadAttribute(attributes, ShipAttributeIds::MaxShield);
		const float disabledRegen = ReadAttribute(attributes, ShipAttributeIds::ShieldRegen);
		enemy->GetShipRuntime().SetSystemEnabled(ShipSystem::Shield, true);
		const float restoredShield = ReadAttribute(attributes, ShipAttributeIds::MaxShield);
		report.Check(NearlyEqual(disabledShield, 0.f) && NearlyEqual(disabledRegen, 0.f) &&
			NearlyEqual(restoredShield, withContribution) && withContribution > 25.f,
			"disabling the shield zeroes it and re-enabling restores level growth and contributions",
			{ { "disabled", disabledShield }, { "restored", restoredShield }, { "expected", withContribution } });
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
	vega::InstallVegaEngineAssets();
	if (argc != 2)
	{
		std::cerr << "Usage: PlayerShipSelectionTests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		vega::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		std::string windowTitle = "Player ship selection E2E";
		vega::Application application{ sf::Vector2u{ 64u, 64u }, 32u, windowTitle, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		RunSelectionKeyChecks(report, application);
		RunPlayerRuntimeChecks(report, application);
		RunEnemyShieldRegression(report, application);
		RunModuleApplierPlayerChecks(report, application);
		RunModuleTestGrantChecks(report, application);
		RunModuleApplierHandleChecks(report, application);
		RunModuleRebootAndClampChecks(report, application);
		RunModuleAbilityAndPrimaryRuleChecks(report, application);
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
