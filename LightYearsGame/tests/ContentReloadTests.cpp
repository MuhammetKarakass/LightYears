#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include <nlohmann/json.hpp>
#include <framework/EngineContext.h>
#include <framework/AssetManager.h>
#include <framework/debug/LogBuffer.h>
#include "gameplay/content/GameContentBootstrap.h"
#include <fstream>
#include <iostream>
#include <algorithm>

int main()
{
	ly::EngineContext context;
	ly::EngineContext::Scope scope{ context };
	using namespace ly::content;
	const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
	const auto root = std::filesystem::path{ CONTENT_RELOAD_TEST_ROOT };
	std::filesystem::create_directories(root);
	int failures = 0;
	const auto check = [&](bool value, const char* text) { if (!value) { ++failures; std::cerr << text << '\n'; } };
	std::string error;
	const auto assets = root / "assets";
	std::filesystem::create_directories(assets / "content/data");
	std::filesystem::copy(source, assets / "content/data", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	context.Assets().SetAssetRootDirectory(assets.generic_u8string() + "/");
	check(ly::GameContentBootstrap::Register(), "Bootstrap fixture failed");
	const auto read = [](const auto& path) { nlohmann::json result; std::ifstream{ path } >> result; return result; };
	const auto fixtureWeapon = read(source / "weapons.json")["weapons"][0]["id"].get<std::string>();
	const auto fixtureShip = ShipContentCatalog::GetPlayerShipIds().front();
	const auto verifyRejected = [&](const char* changed, const auto& candidate)
	{
		const auto* abilityBefore = AbilityContentCatalog::GetDefinitions().front();
		const auto* effectBefore = EffectContentCatalog::GetDefinitions().front();
		const auto* weaponBefore = WeaponContentCatalog::FindById(fixtureWeapon);
		const auto* shipBefore = ShipContentCatalog::FindById(fixtureShip);
		const auto revisionBefore = ModuleContentCatalog::GetRevision();
		std::ofstream{ assets / "content/data" / changed } << candidate;
		ly::debug::LogCapture capture;
		ly::GameContentBootstrap::Reload(context, { assets / "content/data" / changed });
		check(AbilityContentCatalog::GetDefinitions().front() == abilityBefore && EffectContentCatalog::GetDefinitions().front() == effectBefore &&
			WeaponContentCatalog::FindById(fixtureWeapon) == weaponBefore && ShipContentCatalog::FindById(fixtureShip) == shipBefore &&
			ModuleContentCatalog::GetRevision() == revisionBefore, "Rejected dependency edit replaced a catalog");
		const auto logs = capture.Buffer().Snapshot();
		check(std::any_of(logs.begin(), logs.end(), [](const auto& row) { return row.level == ly::debug::LogLevel::Error && row.message.find("Content reload rejected") != std::string::npos; }), "Dependency rejection must reach the Console");
		std::filesystem::copy_file(source / changed, assets / "content/data" / changed, std::filesystem::copy_options::overwrite_existing);
	};
	auto effectsCandidate = read(source / "effects.json");
	for (auto it = effectsCandidate["effects"].begin(); it != effectsCandidate["effects"].end();)
		if (it->at("id") == "Effect.Barrier.Basic") it = effectsCandidate["effects"].erase(it); else ++it;
	verifyRejected("effects.json", effectsCandidate);
	auto weaponsCandidate = read(source / "weapons.json");
	const auto shipsCandidate = read(source / "ships.json");
	std::string primary;
	for (const auto& ship : shipsCandidate["ships"]) if (ship.contains("primaryWeaponId")) { primary = ship["primaryWeaponId"]; break; }
	check(!primary.empty(), "Ship weapon dependency fixture missing");
	for (auto it = weaponsCandidate["weapons"].begin(); it != weaponsCandidate["weapons"].end();)
		if (it->at("id") == primary) it = weaponsCandidate["weapons"].erase(it); else ++it;
	verifyRejected("weapons.json", weaponsCandidate);
	auto modulesCandidate = read(source / "modules.json");
	const auto requiredModule = shipsCandidate["ships"][0]["schematic"]["defaultModules"][0]["moduleId"];
	for (auto it = modulesCandidate["modules"].begin(); it != modulesCandidate["modules"].end();)
		if (it->at("id") == requiredModule) it = modulesCandidate["modules"].erase(it); else ++it;
	verifyRejected("modules.json", modulesCandidate);
	const auto abilityLoad = [&](const auto& path) { return AbilityContentCatalog::LoadFromFile(path, AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions(), &error, true); };
	const auto effectLoad = [&](const auto& path) { return EffectContentCatalog::LoadFromFile(path, EffectData::GetBuiltinGameplayEffectDefinitions(), &error, true); };
	check(abilityLoad(source / "abilities.json"), "Ability initial load failed");
	const auto* ability = AbilityContentCatalog::GetDefinitions().front();
	const auto abilityId = ability->abilityId;
	const auto cooldown = ability->cooldown;
	nlohmann::json document;
	std::ifstream{ source / "abilities.json" } >> document;
	document["abilities"][0]["cooldown"] = cooldown + 1.f;
	std::ofstream{ root / "abilities.json" } << document;
	check(abilityLoad(root / "abilities.json"), "Ability reload failed");
	check(ability->cooldown == cooldown && AbilityContentCatalog::FindById(abilityId)->cooldown == cooldown + 1.f, "Ability reload changed a live definition or missed future lookups");
	const auto* currentAbility = AbilityContentCatalog::FindById(abilityId);
	check(!abilityLoad(root / "missing.json") && AbilityContentCatalog::FindById(abilityId) == currentAbility, "Failed ability reload erased live catalog");
	check(effectLoad(source / "effects.json"), "Effect initial load failed");
	const auto* effect = EffectContentCatalog::GetDefinitions().front();
	const auto effectId = effect->effectId;
	check(effectLoad(source / "effects.json") && effect->effectId == effectId && effect != EffectContentCatalog::FindById(effectId), "Effect generation did not retain old pointer");
	const auto* currentEffect = EffectContentCatalog::FindById(effectId);
	check(!effectLoad(root / "missing.json") && EffectContentCatalog::FindById(effectId) == currentEffect, "Failed effect reload erased live catalog");
	check(WeaponContentCatalog::LoadFromFile(source / "weapons.json", &error, true), "Weapon load failed");
	std::ifstream{ source / "weapons.json" } >> document;
	const auto weaponId = document["weapons"][0]["id"].get<std::string>();
	const auto* weapon = WeaponContentCatalog::FindById(weaponId);
	check(WeaponContentCatalog::LoadFromFile(source / "weapons.json", &error, true) && weapon->weaponId == weaponId && weapon != WeaponContentCatalog::FindById(weaponId), "Weapon generation did not retain old pointer");
	const auto* currentWeapon = WeaponContentCatalog::FindById(weaponId);
	check(!WeaponContentCatalog::LoadFromFile(root / "missing.json", &error, true) && WeaponContentCatalog::FindById(weaponId) == currentWeapon, "Failed weapon reload erased live catalog");
	check(ShipContentCatalog::LoadFromFile(source / "ships.json", &error, true), "Ship load failed");
	std::ifstream{ source / "ships.json" } >> document;
	const auto shipId = document["ships"][0]["id"].get<std::string>();
	const auto* ship = ShipContentCatalog::FindById(shipId);
	const auto health = ship->health;
	document["ships"][0]["health"] = health + 10.f;
	std::ofstream{ root / "ships.json" } << document;
	check(ShipContentCatalog::LoadFromFile(root / "ships.json", &error, true) && ship->health == health && ShipContentCatalog::FindById(shipId)->health == health + 10.f, "Ship reload changed live definition or missed future spawns");
	const auto* currentShip = ShipContentCatalog::FindById(shipId);
	check(!ShipContentCatalog::LoadFromFile(root / "missing.json", &error, true) && ShipContentCatalog::FindById(shipId) == currentShip, "Failed ship reload erased live catalog");
	check(ModuleContentCatalog::LoadFromFiles(source / "modules.json", source / "module_side_features.json", &error), "Module load failed");
	const auto* module = &ModuleContentCatalog::GetDefinitions().front();
	const auto moduleId = module->moduleId;
	check(ModuleContentCatalog::LoadFromFiles(source / "modules.json", source / "module_side_features.json", &error) && module->moduleId == moduleId && module != ModuleContentCatalog::FindById(moduleId), "Module generation did not retain old pointer");
	std::cout << "Content reload: " << failures << " failures\n";
	return failures ? 1 : 0;
}
