#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/combat/EffectConfig.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

int main()
{
	using namespace ly::content;
	const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
	const auto root = std::filesystem::path{ CONTENT_RELOAD_TEST_ROOT };
	std::filesystem::create_directories(root);
	int failures = 0;
	const auto check = [&](bool value, const char* text) { if (!value) { ++failures; std::cerr << text << '\n'; } };
	std::string error;
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
