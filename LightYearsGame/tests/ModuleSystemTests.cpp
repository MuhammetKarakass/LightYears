#include "framework/JsonDocumentLoader.h"
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ShipSchematicValidator.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ly
{
	weak_ptr<Reward> CreateRewardHealth(World*) { return {}; }
	weak_ptr<Reward> CreateRewardLife(World*) { return {}; }
	weak_ptr<Reward> CreateRewardShield(World*) { return {}; }
}

namespace
{
	using Json = ly::JsonDocumentLoader::Json;

	int gFailures = 0;

	void Check(bool condition, const std::string& message)
	{
		if (!condition)
		{
			std::cerr << "FAIL: " << message << '\n';
			++gFailures;
		}
	}

	std::filesystem::path DataPath(const char* fileName)
	{
		return std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / fileName;
	}

	Json ReadJson(const std::filesystem::path& path)
	{
		std::ifstream stream{ path };
		return Json::parse(stream);
	}

	std::filesystem::path WriteTemp(const Json& json, const char* name)
	{
		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "LightYearsModuleSystemTests";
		std::filesystem::create_directories(directory);
		const std::filesystem::path path = directory / name;
		std::ofstream stream{ path };
		stream << json.dump(2);
		return path;
	}

	// ---- modules.json ----

	ly::content::ModuleLoader::Result LoadModules(const Json& json)
	{
		return ly::content::ModuleLoader::LoadFromFile(WriteTemp(json, "modules_case.json"));
	}

	void ExpectModulesRejected(const char* label, const std::function<void(Json&)>& mutate)
	{
		Json json = ReadJson(DataPath("modules.json"));
		mutate(json);
		const auto result = LoadModules(json);
		Check(!result.Succeeded(), std::string("modules.json mutation was accepted: ") + label);
	}

	void TestShippedModules()
	{
		const auto result = ly::content::ModuleLoader::LoadFromFile(DataPath("modules.json"));
		Check(result.Succeeded(), "shipped modules.json failed to load: " + result.error);
		if (!result.Succeeded()) return;

		Check(result.definitions.size() == 19, "shipped modules.json should hold 4 standard system modules and 15 content modules");
		const ly::ModuleZoneTable& table = result.zoneTable;
		Check(table.zones.size() == 6, "zone table should hold six zones");
		Check(table.rebootDurationSeconds == 1.5f && table.combatWindowSeconds == 3.f, "reboot/combat durations mismatch");

		struct ZoneExpectation { ly::GameplayTag zone; const ly::GameplayTag* family; float bonus; };
		const ZoneExpectation zones[] = {
			{ ly::GameplayTags::Module::Zone::Nose, &ly::GameplayTags::Ability::Primary, 0.25f },
			{ ly::GameplayTags::Module::Zone::Wings, &ly::GameplayTags::Ability::Offense, 0.25f },
			{ ly::GameplayTags::Module::Zone::Hull, nullptr, 0.10f },
			{ ly::GameplayTags::Module::Zone::Core, &ly::GameplayTags::Ability::Defense, 0.25f },
			{ ly::GameplayTags::Module::Zone::Engine, &ly::GameplayTags::Ability::Movement, 0.25f },
			{ ly::GameplayTags::Module::Zone::Sensor, &ly::GameplayTags::Ability::Utility, 0.25f }
		};
		for (const ZoneExpectation& expected : zones)
		{
			const ly::ModuleZoneEntry* entry = table.FindZone(expected.zone);
			Check(entry != nullptr, "missing zone " + expected.zone.name);
			if (!entry) continue;
			Check(entry->bonus == expected.bonus, "bonus mismatch for " + expected.zone.name);
			Check(entry->anyFamily == (expected.family == nullptr), "wildcard mismatch for " + expected.zone.name);
			if (expected.family) Check(entry->family == *expected.family, "family mismatch for " + expected.zone.name);
		}

		const auto* shieldZone = table.FindSystemZone(ly::GameplayTags::Module::System::Shield);
		const auto* afterburnerZone = table.FindSystemZone(ly::GameplayTags::Module::System::Afterburner);
		const auto* healthZone = table.FindSystemZone(ly::GameplayTags::Module::System::HealthReadout);
		const auto* damageZone = table.FindSystemZone(ly::GameplayTags::Module::System::DamageReadout);
		Check(shieldZone && *shieldZone == ly::GameplayTags::Module::Zone::Core, "Shield should lock to Core");
		Check(afterburnerZone && *afterburnerZone == ly::GameplayTags::Module::Zone::Engine, "Afterburner should lock to Engine");
		Check(healthZone && *healthZone == ly::GameplayTags::Module::Zone::Sensor, "HealthReadout should lock to Sensor");
		Check(damageZone && *damageZone == ly::GameplayTags::Module::Zone::Sensor, "DamageReadout should lock to Sensor");

		struct ModuleExpectation { const char* id; const ly::GameplayTag* family; const ly::GameplayTag* system; std::size_t cells; };
		const ModuleExpectation modules[] = {
			{ "Module.Defense.ShieldGenerator.Standard", &ly::GameplayTags::Ability::Defense, &ly::GameplayTags::Module::System::Shield, 2 },
			{ "Module.Movement.Afterburner.Standard", &ly::GameplayTags::Ability::Movement, &ly::GameplayTags::Module::System::Afterburner, 2 },
			{ "Module.Utility.HealthReadout.Standard", &ly::GameplayTags::Ability::Utility, &ly::GameplayTags::Module::System::HealthReadout, 1 },
			{ "Module.Utility.DamageReadout.Standard", &ly::GameplayTags::Ability::Utility, &ly::GameplayTags::Module::System::DamageReadout, 1 }
		};
		for (const ModuleExpectation& expected : modules)
		{
			const auto found = std::find_if(result.definitions.begin(), result.definitions.end(),
				[&](const ly::ModuleDefinition& module) { return module.moduleId.ToString() == expected.id; });
			Check(found != result.definitions.end(), std::string("missing module ") + expected.id);
			if (found == result.definitions.end()) continue;
			Check(found->family == *expected.family, std::string("family mismatch ") + expected.id);
			Check(found->systemCategory == *expected.system, std::string("system mismatch ") + expected.id);
			Check(found->sizeVariants.size() == 1 && found->sizeVariants[0].shape.cells.size() == expected.cells,
				std::string("variant/shape mismatch ") + expected.id);
			Check(found->sizeVariants[0].attributeModifiers.empty() && found->sizeVariants[0].abilityModifiers.empty(),
				std::string("system module should carry no modifiers ") + expected.id);
		}

		std::string failure;
		Check(ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "catalog load failed: " + failure);
		Check(ly::content::ModuleContentCatalog::IsLoaded(), "catalog should report loaded");
		Check(ly::content::ModuleContentCatalog::FindById("Module.Defense.ShieldGenerator.Standard") != nullptr, "catalog lookup failed");
		Check(ly::content::ModuleContentCatalog::FindById("Module.Defense.Nope.Standard") == nullptr, "catalog lookup should miss");
	}

	void TestIdentities()
	{
		using ly::content::ContentIdSchema;
		const char* valid[] = {
			"Module.Primary.FocusedBarrel.Standard", "Module.Offense.AttackMatrix.A", "Module.Defense.ArmorPlate.Two",
			"Module.Movement.Thruster.Standard", "Module.Utility.CoolingLoop.Standard"
		};
		for (const char* id : valid) Check(ContentIdSchema::ValidateModuleId(id), std::string("valid module ID rejected: ") + id);
		const char* invalid[] = {
			"", "Module", "Module.Defense", "Module.Defense.Armor", "Module.Defense.Armor.Std.Extra", "Module.Control.Armor.Std",
			"Module.Zone.Armor.Std", "Ability.Defense.Armor.Std", "Module..Armor.Std", "Module.Defense.Arm or.Std", "module.Defense.Armor.Std"
		};
		for (const char* id : invalid) Check(!ContentIdSchema::ValidateModuleId(id), std::string("invalid module ID accepted: ") + id);

		using ly::GameplayTagKind;
		using ly::GameplayTagSchema;
		const char* zones[] = { "Module.Zone.Nose", "Module.Zone.Wings", "Module.Zone.Hull", "Module.Zone.Core", "Module.Zone.Engine", "Module.Zone.Sensor" };
		for (const char* zone : zones)
		{
			Check(GameplayTagSchema::Validate(ly::GameplayTag{ zone }, GameplayTagKind::ModuleZone), std::string("zone tag rejected: ") + zone);
			Check(!GameplayTagSchema::Validate(ly::GameplayTag{ zone }, GameplayTagKind::ModuleSystem), std::string("zone accepted as system: ") + zone);
		}
		const char* systems[] = { "Module.System.Shield", "Module.System.Afterburner", "Module.System.HealthReadout", "Module.System.DamageReadout" };
		for (const char* system : systems) Check(GameplayTagSchema::Validate(ly::GameplayTag{ system }, GameplayTagKind::ModuleSystem), std::string("system tag rejected: ") + system);
		Check(!GameplayTagSchema::Validate(ly::GameplayTag{ "Module.Zone.Cockpit" }, GameplayTagKind::ModuleZone), "unknown zone tag accepted");
		Check(!GameplayTagSchema::Validate(ly::GameplayTag{ "Module.System.Radar" }, GameplayTagKind::ModuleSystem), "unknown system tag accepted");
		Check(!GameplayTagSchema::Validate(ly::GameplayTag{ "Module.Zone" }, GameplayTagKind::ModuleZone), "zone root accepted");
	}

	void TestModuleRejections()
	{
		ExpectModulesRejected("bad id grammar", [](Json& j) { j["modules"][0]["id"] = "Module.Defense.ShieldGenerator"; });
		ExpectModulesRejected("unknown id family", [](Json& j) { j["modules"][0]["id"] = "Module.Control.ShieldGenerator.Standard"; });
		ExpectModulesRejected("family does not match id", [](Json& j) { j["modules"][0]["family"] = "Ability.Offense"; });
		ExpectModulesRejected("family not a category", [](Json& j) { j["modules"][0]["family"] = "Ability.Defense.ShieldGenerator"; });
		ExpectModulesRejected("family unknown category", [](Json& j) { j["modules"][0]["family"] = "Ability.Control"; });
		ExpectModulesRejected("unknown system tag", [](Json& j) { j["modules"][0]["systemCategory"] = "Module.System.Radar"; });
		ExpectModulesRejected("zone tag as system", [](Json& j) { j["modules"][0]["systemCategory"] = "Module.Zone.Core"; });
		ExpectModulesRejected("unknown zone tag", [](Json& j) { j["zones"][0]["zone"] = "Module.Zone.Cockpit"; });
		ExpectModulesRejected("duplicate zone", [](Json& j) { j["zones"][1]["zone"] = "Module.Zone.Nose"; });
		ExpectModulesRejected("missing zone", [](Json& j) { j["zones"].erase(j["zones"].begin() + 2); });
		ExpectModulesRejected("zone family not a category", [](Json& j) { j["zones"][0]["family"] = "Module.Zone.Nose"; });
		ExpectModulesRejected("negative bonus", [](Json& j) { j["zones"][0]["bonus"] = -0.25; });
		ExpectModulesRejected("non-finite bonus", [](Json& j) { j["zones"][0]["bonus"] = 1e39; });
		ExpectModulesRejected("unknown systemZone key", [](Json& j) { j["systemZones"]["Module.System.Radar"] = "Module.Zone.Core"; });
		ExpectModulesRejected("unknown systemZone zone", [](Json& j) { j["systemZones"]["Module.System.Shield"] = "Module.Zone.Cockpit"; });
		ExpectModulesRejected("missing systemZone", [](Json& j) { j["systemZones"].erase("Module.System.Shield"); });
		ExpectModulesRejected("non-finite reboot", [](Json& j) { j["rebootDurationSeconds"] = 1e39; });
		ExpectModulesRejected("zero combat window", [](Json& j) { j["combatWindowSeconds"] = 0.0; });
		ExpectModulesRejected("empty sizeVariants", [](Json& j) { j["modules"][0]["sizeVariants"] = Json::array(); });
		ExpectModulesRejected("missing sizeVariants", [](Json& j) { j["modules"][0].erase("sizeVariants"); });
		ExpectModulesRejected("empty shape", [](Json& j) { j["modules"][0]["sizeVariants"][0]["shape"] = Json::array(); });
		ExpectModulesRejected("duplicate cells", [](Json& j) { j["modules"][0]["sizeVariants"][0]["shape"] = Json::parse("[[0,0],[0,0]]"); });
		ExpectModulesRejected("negative cell", [](Json& j) { j["modules"][0]["sizeVariants"][0]["shape"] = Json::parse("[[0,0],[-1,0]]"); });
		ExpectModulesRejected("malformed cell", [](Json& j) { j["modules"][0]["sizeVariants"][0]["shape"] = Json::parse("[[0,0,1]]"); });
		ExpectModulesRejected("duplicate module id", [](Json& j) { j["modules"].push_back(j["modules"][0]); });
		ExpectModulesRejected("non-finite modifier", [](Json& j)
		{
			j["modules"][0]["sizeVariants"][0]["attributeModifiers"] = Json::parse(
				R"([{"target":"Ship","attributeId":"Owner.MaxShield","operation":"Add","magnitude":1e39}])");
		});
		ExpectModulesRejected("unknown modifier target", [](Json& j)
		{
			j["modules"][0]["sizeVariants"][0]["attributeModifiers"] = Json::parse(
				R"([{"target":"Cargo","attributeId":"Owner.MaxShield","operation":"Add","magnitude":1}])");
		});

		Json valid = ReadJson(DataPath("modules.json"));
		valid["modules"][0]["sizeVariants"][0]["attributeModifiers"] = Json::parse(
			R"([{"target":"Ship","attributeId":"Owner.MaxShield","operation":"Multiply","magnitude":0.8,"affectedByZoneBonus":false}])");
		valid["modules"][0]["sizeVariants"][0]["abilityModifiers"] = Json::parse(
			R"([{"attributeId":"Common.Damage","operation":"Multiply","magnitude":1.12}])");
		const auto accepted = LoadModules(valid);
		Check(accepted.Succeeded(), "valid modifiers were rejected: " + accepted.error);
		if (accepted.Succeeded())
		{
			const ly::ModuleSizeVariant& variant = accepted.definitions[0].sizeVariants[0];
			Check(variant.attributeModifiers.size() == 1 && variant.attributeModifiers[0].target == ly::ModuleAttributeTarget::Ship &&
				!variant.attributeModifiers[0].affectedByZoneBonus, "attribute modifier did not round-trip");
			Check(variant.abilityModifiers.size() == 1 && variant.abilityModifiers[0].affectedByZoneBonus, "ability modifier did not round-trip");
		}
	}

	// ---- ships.json schematics ----

	ShipDefinition MakeBaseShip()
	{
		return ShipDefinition{ "", 100.f, sf::Vector2f{ 0.f, 0.f }, 0.f, 0.f, 0, {}, {} };
	}

	ly::content::ShipLoader::Result LoadShips(const Json& json)
	{
		return ly::content::ShipLoader::LoadFromFile(WriteTemp(json, "ships_case.json"), MakeBaseShip());
	}

	void ExpectShipsRejected(const char* label, const std::function<void(Json&)>& mutate)
	{
		Json json = ReadJson(DataPath("ships.json"));
		mutate(json);
		Check(!LoadShips(json).Succeeded(), std::string("ships.json mutation was accepted: ") + label);
	}

	Json& PlayerSchematic(Json& json, std::size_t shipIndex)
	{
		return json["ships"][shipIndex]["schematic"];
	}

	void TestShippedSchematics()
	{
		const auto ships = ly::content::ShipLoader::LoadFromFile(DataPath("ships.json"), MakeBaseShip());
		Check(ships.Succeeded(), "shipped ships.json failed to load: " + ships.error);
		if (!ships.Succeeded()) return;

		std::string failure;
		Check(ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "module catalog load failed: " + failure);

		struct Expectation { const char* id; int nose, wings, hull, core, engine, sensor; };
		const Expectation expectations[] = {
			{ "Ship.Player.Fighter.Basic", 4, 8, 4, 6, 4, 0 },
			{ "Ship.Player.Breacher.Basic", 6, 8, 4, 2, 4, 2 },
			{ "Ship.Player.Interceptor.Basic", 4, 4, 2, 2, 10, 4 },
			{ "Ship.Player.Conductor.Basic", 2, 6, 6, 4, 4, 4 },
			{ "Ship.Player.Aegis.Basic", 2, 4, 4, 12, 2, 2 },
			{ "Ship.Player.CryoController.Basic", 4, 6, 4, 4, 4, 4 }
		};

		std::size_t shipsWithSchematic = 0;
		for (const auto& loaded : ships.definitions)
		{
			if (loaded.id.rfind("Ship.Enemy.", 0) == 0)
			{
				Check(!loaded.definition.schematic.has_value(), "enemy " + loaded.id + " should have no schematic");
			}
			if (loaded.definition.schematic) ++shipsWithSchematic;
		}
		Check(shipsWithSchematic == 6, "exactly the six player ships should carry a schematic");

		for (const Expectation& expected : expectations)
		{
			const auto found = std::find_if(ships.definitions.begin(), ships.definitions.end(),
				[&](const auto& loaded) { return loaded.id == expected.id; });
			Check(found != ships.definitions.end() && found->definition.schematic.has_value(), std::string("no schematic for ") + expected.id);
			if (found == ships.definitions.end() || !found->definition.schematic) continue;
			const ly::ShipSchematic& schematic = *found->definition.schematic;

			std::string schematicFailure;
			Check(ly::ShipSchematicValidator::Validate(schematic, ly::content::ModuleContentCatalog::GetDefinitions(),
				ly::content::ModuleContentCatalog::GetZoneTable(), &schematicFailure),
				std::string(expected.id) + " schematic invalid: " + schematicFailure);

			std::map<std::string, int> counts;
			for (const ly::GameplayTag& zone : schematic.cellZones) if (zone.IsValid()) ++counts[zone.name];
			std::set<std::pair<int, int>> defaultCells;
			std::set<std::string> systems;
			for (const ly::DefaultModulePlacement& placement : schematic.defaultModules)
			{
				const ly::ModuleDefinition* module = ly::content::ModuleContentCatalog::FindById(placement.moduleId);
				Check(module != nullptr, "default module missing from catalog");
				if (!module) continue;
				systems.insert(module->systemCategory.name);
				for (const sf::Vector2i& cell : ly::ModuleShapeRules::Rotate(module->sizeVariants[0].shape.cells, placement.rotationQuarterTurns))
				{
					const int x = placement.origin.x + cell.x;
					const int y = placement.origin.y + cell.y;
					Check(defaultCells.insert({ x, y }).second, "default modules overlap");
					--counts[schematic.GetZoneAt(x, y).name];
				}
			}
			Check(schematic.defaultModules.size() == 4 && systems.size() == 4, std::string(expected.id) + " should default all four system modules");

			const int expectedFree[] = { expected.nose, expected.wings, expected.hull, expected.core, expected.engine, expected.sensor };
			const ly::GameplayTag zoneOrder[] = {
				ly::GameplayTags::Module::Zone::Nose, ly::GameplayTags::Module::Zone::Wings, ly::GameplayTags::Module::Zone::Hull,
				ly::GameplayTags::Module::Zone::Core, ly::GameplayTags::Module::Zone::Engine, ly::GameplayTags::Module::Zone::Sensor
			};
			int total = 0;
			for (int index = 0; index < 6; ++index)
			{
				Check(counts[zoneOrder[index].name] == expectedFree[index],
					std::string(expected.id) + " free cells in " + zoneOrder[index].name + ": got " +
					std::to_string(counts[zoneOrder[index].name]) + ", expected " + std::to_string(expectedFree[index]));
				total += counts[zoneOrder[index].name];
			}
			Check(total == ly::ShipSchematicValidator::kFreeCellCapacity, std::string(expected.id) + " free cell total is not the shipped capacity");
			Check(schematic.width >= 6 && schematic.width <= 8, std::string(expected.id) + " grid width out of 6..8");
			// Every shipped schematic is mirror-symmetric about its vertical centre.
			bool symmetric = true;
			for (int y = 0; y < schematic.height; ++y)
				for (int x = 0; x < schematic.width / 2; ++x)
					if (schematic.GetZoneAt(x, y) != schematic.GetZoneAt(schematic.width - 1 - x, y)) symmetric = false;
			Check(symmetric, std::string(expected.id) + " schematic rows are mirror-symmetric");
		}
	}

	void TestShipLoaderRejections()
	{
		ExpectShipsRejected("character missing from legend", [](Json& j) { PlayerSchematic(j, 0)["rows"][0] = "..XXX.."; });
		ExpectShipsRejected("ragged rows", [](Json& j) { PlayerSchematic(j, 0)["rows"][1] = "...N.."; });
		ExpectShipsRejected("unknown zone tag in legend", [](Json& j) { PlayerSchematic(j, 0)["legend"]["N"] = "Module.Zone.Cockpit"; });
		ExpectShipsRejected("system tag in legend", [](Json& j) { PlayerSchematic(j, 0)["legend"]["N"] = "Module.System.Shield"; });
		ExpectShipsRejected("dot legend key", [](Json& j) { PlayerSchematic(j, 0)["legend"]["."] = "Module.Zone.Nose"; });
		ExpectShipsRejected("empty rows", [](Json& j) { PlayerSchematic(j, 0)["rows"] = Json::array(); });
		ExpectShipsRejected("bad default module id", [](Json& j) { PlayerSchematic(j, 0)["defaultModules"][0]["moduleId"] = "Module.Defense.Shield"; });
		ExpectShipsRejected("rotation out of range", [](Json& j) { PlayerSchematic(j, 0)["defaultModules"][0]["rotation"] = 4; });
		ExpectShipsRejected("malformed origin", [](Json& j) { PlayerSchematic(j, 0)["defaultModules"][0]["origin"] = Json::parse("[1]"); });

		Json json = ReadJson(DataPath("ships.json"));
		PlayerSchematic(json, 0)["defaultModules"][0]["rotation"] = 1;
		const auto rotated = LoadShips(json);
		Check(rotated.Succeeded() && rotated.definitions[0].definition.schematic->defaultModules[0].rotationQuarterTurns == 1,
			"valid rotation was not preserved");
	}

	void TestSchematicValidatorRejections()
	{
		const auto ships = ly::content::ShipLoader::LoadFromFile(DataPath("ships.json"), MakeBaseShip());
		std::string failure;
		Check(ships.Succeeded() && ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "setup failed");
		if (!ships.Succeeded() || !ships.definitions[0].definition.schematic) return;

		const ly::ShipSchematic fighter = *ships.definitions[0].definition.schematic;
		const auto& modules = ly::content::ModuleContentCatalog::GetDefinitions();
		const auto& zones = ly::content::ModuleContentCatalog::GetZoneTable();
		const auto validate = [&](const ly::ShipSchematic& schematic) { return ly::ShipSchematicValidator::Validate(schematic, modules, zones); };
		const auto expectRejected = [&](const char* label, const std::function<void(ly::ShipSchematic&)>& mutate)
		{
			ly::ShipSchematic copy = fighter;
			mutate(copy);
			Check(!validate(copy), std::string("schematic mutation was accepted: ") + label);
		};

		Check(validate(fighter), "unmodified fighter schematic should validate");
		expectRejected("unknown module", [](ly::ShipSchematic& s) { s.defaultModules[0].moduleId = "Module.Defense.Nope.Standard"; });
		expectRejected("invalid variant", [](ly::ShipSchematic& s) { s.defaultModules[0].variantIndex = 1; });
		expectRejected("negative variant", [](ly::ShipSchematic& s) { s.defaultModules[0].variantIndex = -1; });
		expectRejected("rotation out of range", [](ly::ShipSchematic& s) { s.defaultModules[0].rotationQuarterTurns = 4; });
		const auto shieldIndex = [](const ly::ShipSchematic& s)
		{
			for (std::size_t i = 0; i < s.defaultModules.size(); ++i)
				if (s.defaultModules[i].moduleId.ToString() == "Module.Defense.ShieldGenerator.Standard") return i;
			return std::size_t{ 0 };
		};
		expectRejected("shield outside Core", [&](ly::ShipSchematic& s) { s.defaultModules[shieldIndex(s)].origin = sf::Vector2i{ 3, 2 }; });
		expectRejected("out of bounds", [](ly::ShipSchematic& s) { s.defaultModules[1].origin = sf::Vector2i{ 6, 7 }; });
		expectRejected("negative origin", [](ly::ShipSchematic& s) { s.defaultModules[1].origin = sf::Vector2i{ -1, 7 }; });
		expectRejected("overlap", [](ly::ShipSchematic& s) { s.defaultModules[3].origin = s.defaultModules[2].origin; });
		expectRejected("duplicate system", [](ly::ShipSchematic& s) { s.defaultModules[3].moduleId = s.defaultModules[2].moduleId; s.defaultModules[3].origin = sf::Vector2i{ 3, 2 }; });
		expectRejected("rotation leaves zone", [&](ly::ShipSchematic& s) { s.defaultModules[shieldIndex(s)].rotationQuarterTurns = 1; });
		expectRejected("27 free cells", [](ly::ShipSchematic& s) { s.cellZones[static_cast<std::size_t>(0 * s.width + 3)] = ly::GameplayTag{}; });
		expectRejected("29 free cells", [](ly::ShipSchematic& s) { s.cellZones[0] = ly::GameplayTags::Module::Zone::Nose; });
		expectRejected("unknown zone in grid", [](ly::ShipSchematic& s) { s.cellZones[2] = ly::GameplayTag{ "Module.Zone.Cockpit" }; });
		expectRejected("grid size mismatch", [](ly::ShipSchematic& s) { s.cellZones.pop_back(); });

		// A rotated (vertical) shield domino fits when the Core has room below.
		ly::ShipSchematic vertical = fighter;
		vertical.defaultModules[shieldIndex(vertical)].origin = sf::Vector2i{ 5, 4 };
		vertical.defaultModules[shieldIndex(vertical)].rotationQuarterTurns = 1;
		Check(validate(vertical), "a rotated default module that fits the Core should validate");
	}

	// ---- placement rules, loadout, resolver (hand-built data) ----

	using ly::ModulePlacementFailure;
	namespace Tags = ly::GameplayTags;

	// 5x4 grid: N nose, W wings, H hull, C core, E engine, S sensor, . void.
	//   x: 0 1 2 3 4
	// y0:  N N W W .
	// y1:  N H H W .
	// y2:  C C H E E
	// y3:  S S . E E
	struct TestWorld
	{
		ly::ShipSchematic schematic;
		ly::ModuleZoneTable zones;
		ly::List<ly::ModuleDefinition> modules;

		static ly::ModuleDefinition Make(const char* id, const ly::GameplayTag& family, const ly::GameplayTag& system, std::initializer_list<sf::Vector2i> cells)
		{
			ly::ModuleDefinition module;
			module.moduleId = id;
			module.family = family;
			module.systemCategory = system;
			ly::ModuleSizeVariant variant;
			variant.shape.cells = ly::List<sf::Vector2i>(cells);
			module.sizeVariants.push_back(variant);
			return module;
		}

		TestWorld()
		{
			const ly::GameplayTag N = Tags::Module::Zone::Nose, W = Tags::Module::Zone::Wings, H = Tags::Module::Zone::Hull;
			const ly::GameplayTag C = Tags::Module::Zone::Core, E = Tags::Module::Zone::Engine, S = Tags::Module::Zone::Sensor;
			const ly::GameplayTag V;
			schematic.width = 5;
			schematic.height = 4;
			schematic.cellZones = { N, N, W, W, V,
									N, H, H, W, V,
									C, C, H, E, E,
									S, S, V, E, E };
			zones.zones = {
				{ N, Tags::Ability::Primary, false, 0.25f }, { W, Tags::Ability::Offense, false, 0.25f }, { H, ly::GameplayTag{}, true, 0.10f },
				{ C, Tags::Ability::Defense, false, 0.25f }, { E, Tags::Ability::Movement, false, 0.25f }, { S, Tags::Ability::Utility, false, 0.25f }
			};
			zones.systemZones = { { Tags::Module::System::Shield, C }, { Tags::Module::System::Afterburner, E } };

			const ly::GameplayTag none;
			modules.push_back(Make("Module.Defense.ShieldGenerator.Standard", Tags::Ability::Defense, Tags::Module::System::Shield, { { 0, 0 }, { 1, 0 } }));
			modules.push_back(Make("Module.Defense.ShieldGenerator.Other", Tags::Ability::Defense, Tags::Module::System::Shield, { { 0, 0 } }));
			modules.push_back(Make("Module.Offense.Matrix.Dot", Tags::Ability::Offense, none, { { 0, 0 } }));
			modules.push_back(Make("Module.Offense.Matrix.L", Tags::Ability::Offense, none, { { 0, 0 }, { 1, 0 }, { 1, 1 } }));
			modules.push_back(Make("Module.Primary.Barrel.Dot", Tags::Ability::Primary, none, { { 0, 0 } }));
			modules.push_back(Make("Module.Offense.Matrix.Domino", Tags::Ability::Offense, none, { { 0, 0 }, { 1, 0 } }));
		}

		ly::ModuleRulesContext Rules() const
		{
			return ly::ModuleRulesContext{ schematic, [this](const sas::ContentId& id) -> const ly::ModuleDefinition*
			{
				for (const ly::ModuleDefinition& module : modules) if (module.moduleId == id) return &module;
				return nullptr;
			}, zones };
		}
	};

	ly::ModulePlacement MakePlacement(const char* id, int x, int y, int rotation = 0, int variant = 0, std::uint32_t instance = 99)
	{
		ly::ModulePlacement placement;
		placement.instanceId = instance;
		placement.moduleId = id;
		placement.variantIndex = variant;
		placement.origin = sf::Vector2i{ x, y };
		placement.rotationQuarterTurns = rotation;
		return placement;
	}

	void TestCanPlace()
	{
		const TestWorld world;
		const auto rules = world.Rules();
		const ly::ModuleLoadout empty;
		const auto check = [&](const char* label, const ly::ModulePlacement& placement, ModulePlacementFailure expected)
		{
			Check(ly::ModulePlacementRules::CanPlace(rules, empty, placement) == expected, std::string("CanPlace: ") + label);
		};
		const char* dot = "Module.Offense.Matrix.Dot";
		const char* lShape = "Module.Offense.Matrix.L";
		const char* shieldStandard = "Module.Defense.ShieldGenerator.Standard";
		check("in bounds", MakePlacement(dot, 2, 0), ModulePlacementFailure::None);
		check("negative origin", MakePlacement(dot, -1, 0), ModulePlacementFailure::OutOfBounds);
		check("past width", MakePlacement(dot, 5, 0), ModulePlacementFailure::OutOfBounds);
		check("past height", MakePlacement(dot, 0, 4), ModulePlacementFailure::OutOfBounds);
		check("void cell", MakePlacement(dot, 4, 0), ModulePlacementFailure::CellBlocked);
		check("unknown module", MakePlacement("Module.Offense.Nope.Dot", 0, 0), ModulePlacementFailure::UnknownModule);
		check("bad variant", MakePlacement(dot, 0, 0, 0, 1), ModulePlacementFailure::InvalidVariant);
		check("negative variant", MakePlacement(dot, 0, 0, 0, -1), ModulePlacementFailure::InvalidVariant);
		check("bad rotation", MakePlacement(dot, 0, 0, 4), ModulePlacementFailure::InvalidRotation);
		check("system in its zone", MakePlacement(shieldStandard, 0, 2), ModulePlacementFailure::None);
		check("system outside its zone", MakePlacement(shieldStandard, 1, 1), ModulePlacementFailure::SystemZoneMismatch);
		check("system straddles zones", MakePlacement(shieldStandard, 1, 2), ModulePlacementFailure::SystemZoneMismatch);

		// L = {(0,0),(1,0),(1,1)}; each quarter turn drops a different corner of its 2x2 block.
		// Block at (3,0) is half void, so every rotation hits a void cell.
		for (int turn = 0; turn < 4; ++turn)
		{
			check("L rejected on void block", MakePlacement(lShape, 3, 0, turn), ModulePlacementFailure::CellBlocked);
			check("L accepted on solid block", MakePlacement(lShape, 0, 2, turn), ModulePlacementFailure::None);
		}
		// Block at (2,2) has its only void cell at (2,3): rotation 0 avoids it, the others cover it.
		check("L rot0 avoids the void corner", MakePlacement(lShape, 2, 2, 0), ModulePlacementFailure::None);
		for (int turn = 1; turn < 4; ++turn)
		{
			check("L rot1-3 cover the void corner", MakePlacement(lShape, 2, 2, turn), ModulePlacementFailure::CellBlocked);
		}
		// Block at (1,3) hangs off the bottom edge.
		check("L rot0 hits void before the edge", MakePlacement(lShape, 1, 3, 0), ModulePlacementFailure::CellBlocked);
		check("L rot2 leaves the grid", MakePlacement(lShape, 1, 3, 2), ModulePlacementFailure::OutOfBounds);
		check("L rot3 leaves the grid", MakePlacement(lShape, 1, 3, 3), ModulePlacementFailure::OutOfBounds);

		ly::ModuleLoadout loadout;
		const std::uint32_t dotId = loadout.AddToInventory(dot);
		Check(loadout.Install(rules, dotId, sf::Vector2i{ 2, 0 }, 0) == ModulePlacementFailure::None, "install dot");
		const std::uint32_t shieldId = loadout.AddToInventory(shieldStandard);
		Check(loadout.Install(rules, shieldId, sf::Vector2i{ 0, 2 }, 0) == ModulePlacementFailure::None, "install shield");

		const auto onLoadout = [&](const ly::ModulePlacement& placement, std::uint32_t ignored = 0)
		{
			return ly::ModulePlacementRules::CanPlace(rules, loadout, placement, ignored);
		};
		Check(onLoadout(MakePlacement(dot, 2, 0)) == ModulePlacementFailure::Overlap, "overlap with installed dot");
		Check(onLoadout(MakePlacement(dot, 2, 0), dotId) == ModulePlacementFailure::None, "ignoring the dot itself frees its cell");
		Check(onLoadout(MakePlacement(dot, 3, 0), dotId) == ModulePlacementFailure::None, "move of the dot to a free cell");
		Check(onLoadout(MakePlacement("Module.Defense.ShieldGenerator.Other", 0, 2)) == ModulePlacementFailure::Overlap, "overlap with the shield");
		Check(onLoadout(MakePlacement("Module.Defense.ShieldGenerator.Other", 2, 2)) == ModulePlacementFailure::SystemZoneMismatch, "second shield outside Core");
		Check(onLoadout(MakePlacement("Module.Defense.ShieldGenerator.Other", 1, 2), shieldId) == ModulePlacementFailure::None, "replacing the shield ignores its own system slot");
	}

	void TestDuplicateSystem()
	{
		TestWorld world;
		// Widen the Core so a second shield module has a legal cell of its own.
		world.schematic.cellZones[static_cast<std::size_t>(1 * world.schematic.width + 0)] = Tags::Module::Zone::Core;
		const auto rules = world.Rules();
		ly::ModuleLoadout loadout;
		const std::uint32_t first = loadout.AddToInventory("Module.Defense.ShieldGenerator.Standard");
		Check(loadout.Install(rules, first, sf::Vector2i{ 0, 2 }, 0) == ModulePlacementFailure::None, "first shield installs");
		const std::uint32_t second = loadout.AddToInventory("Module.Defense.ShieldGenerator.Other");
		Check(loadout.Install(rules, second, sf::Vector2i{ 0, 1 }, 0) == ModulePlacementFailure::DuplicateSystem, "second shield of the same system is rejected");
		Check(loadout.GetPlacements().size() == 1, "duplicate system left placements unchanged");
	}

	void TestLoadoutMutations()
	{
		const TestWorld world;
		const auto rules = world.Rules();
		ly::ModuleLoadout loadout;
		std::uint64_t revision = loadout.GetRevision();
		const auto bumped = [&](const char* label)
		{
			Check(loadout.GetRevision() == revision + 1, std::string("revision should bump once: ") + label);
			revision = loadout.GetRevision();
		};
		const auto unchanged = [&](const char* label)
		{
			Check(loadout.GetRevision() == revision, std::string("revision should not change: ") + label);
		};

		const std::uint32_t a = loadout.AddToInventory("Module.Offense.Matrix.Dot");
		bumped("add a");
		const std::uint32_t b = loadout.AddToInventory("Module.Primary.Barrel.Dot");
		bumped("add b");
		Check(a != 0 && b > a, "instance ids are monotonic");

		Check(loadout.Install(rules, a, sf::Vector2i{ 4, 0 }, 0) == ModulePlacementFailure::CellBlocked, "illegal install rejected");
		unchanged("illegal install");
		Check(loadout.GetPlacements().empty() && loadout.GetInventory().size() == 2, "illegal install left state alone");
		Check(loadout.Install(rules, 12345, sf::Vector2i{ 2, 0 }, 0) == ModulePlacementFailure::UnknownModule, "unknown instance rejected");
		unchanged("unknown instance install");

		Check(loadout.Install(rules, a, sf::Vector2i{ 2, 0 }, 0) == ModulePlacementFailure::None, "install a");
		bumped("install a");
		Check(loadout.GetPlacements().size() == 1 && loadout.GetInventory().size() == 1 && loadout.FindPlacement(a) != nullptr,
			"install moves the item out of inventory");

		Check(loadout.Install(rules, b, sf::Vector2i{ 2, 0 }, 0) == ModulePlacementFailure::Overlap, "overlapping install rejected");
		unchanged("overlapping install");
		Check(loadout.Install(rules, b, sf::Vector2i{ 0, 0 }, 0) == ModulePlacementFailure::None, "install b");
		bumped("install b");

		Check(loadout.Move(rules, a, sf::Vector2i{ 0, 0 }, 0) == ModulePlacementFailure::Overlap, "move onto b rejected");
		unchanged("illegal move");
		Check(loadout.FindPlacement(a)->origin == sf::Vector2i(2, 0), "illegal move left the origin alone");
		Check(loadout.Move(rules, a, sf::Vector2i{ 2, 0 }, 0) == ModulePlacementFailure::None, "move onto itself is legal");
		bumped("move onto itself");
		Check(loadout.Move(rules, a, sf::Vector2i{ 3, 0 }, 0) == ModulePlacementFailure::None, "move to a free cell");
		bumped("move");
		Check(loadout.FindPlacement(a)->origin == sf::Vector2i(3, 0), "move updated the origin");
		Check(loadout.Move(rules, 777, sf::Vector2i{ 3, 0 }, 0) == ModulePlacementFailure::UnknownModule, "move of an unknown instance");
		unchanged("unknown move");

		Check(loadout.Uninstall(a), "uninstall a");
		bumped("uninstall");
		Check(loadout.FindPlacement(a) == nullptr && loadout.FindInventoryItem(a) != nullptr, "uninstall returns the item to inventory");
		Check(!loadout.Uninstall(a), "uninstalling an inventory item fails");
		unchanged("failed uninstall");
		Check(loadout.Remove(a), "remove from inventory");
		bumped("remove from inventory");
		Check(loadout.Remove(b), "remove a placement");
		bumped("remove a placement");
		Check(!loadout.Remove(b), "double remove fails");
		unchanged("failed remove");
		Check(loadout.AddToInventory("Module.Offense.Matrix.Dot") > b, "instance ids are never reused");
	}

	void TestDefaultSeeding()
	{
		const auto ships = ly::content::ShipLoader::LoadFromFile(DataPath("ships.json"), MakeBaseShip());
		std::string failure;
		Check(ships.Succeeded() && ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "seeding setup failed");
		if (!ships.Succeeded()) return;

		const auto catalogLookup = [](const sas::ContentId& id) { return ly::content::ModuleContentCatalog::FindById(id); };
		std::size_t seeded = 0;
		for (const auto& loaded : ships.definitions)
		{
			if (!loaded.definition.schematic) continue;
			const ly::ShipSchematic& schematic = *loaded.definition.schematic;
			const ly::ModuleRulesContext rules{ schematic, catalogLookup, ly::content::ModuleContentCatalog::GetZoneTable() };
			ly::ModuleLoadout loadout;
			Check(loadout.SeedDefaults(rules), "default seeding failed for " + loaded.id);
			Check(loadout.GetPlacements().size() == schematic.defaultModules.size() && loadout.GetInventory().empty(), "seeded placement count for " + loaded.id);
			for (const ly::ModulePlacement& placement : loadout.GetPlacements())
			{
				Check(ly::ModulePlacementRules::CanPlace(rules, loadout, placement, placement.instanceId) == ModulePlacementFailure::None,
					"seeded placement is illegal for " + loaded.id);
			}

			const ly::ResolvedModuleEffects effects = ly::ModuleResolver::Resolve(rules, loadout.GetPlacements());
			std::set<std::string> systems;
			for (const ly::GameplayTag& system : effects.enabledSystems) systems.insert(system.name);
			Check(effects.enabledSystems.size() == 4 && systems.size() == 4, "default loadout should enable all four systems for " + loaded.id);
			Check(effects.ownerModifiers.empty() && effects.shipModifiers.empty() && effects.primaryWeaponModifiers.empty() && effects.abilityRules.empty(),
				"default system modules should add no modifiers for " + loaded.id);
			++seeded;
		}
		Check(seeded == 6, "six player ships should seed");

		// An illegal default leaves the loadout empty.
		ly::ShipSchematic broken = *ships.definitions[0].definition.schematic;
		broken.defaultModules[0].origin = sf::Vector2i{ 0, 0 };
		const ly::ModuleRulesContext rules{ broken, catalogLookup, ly::content::ModuleContentCatalog::GetZoneTable() };
		ly::ModuleLoadout loadout;
		Check(!loadout.SeedDefaults(rules), "illegal defaults should fail seeding");
		Check(loadout.GetPlacements().empty() && loadout.GetInventory().empty(), "failed seeding leaves the loadout empty");
	}

	ly::ModuleAttributeModifier Attr(ly::ModuleAttributeTarget target, const char* attribute, sas::AttributeModifierOperation operation, float magnitude, bool scaled = true)
	{
		ly::ModuleAttributeModifier entry;
		entry.target = target;
		entry.modifier.attributeId = sas::AttributeId{ attribute };
		entry.modifier.operation = operation;
		entry.modifier.magnitude = magnitude;
		entry.affectedByZoneBonus = scaled;
		return entry;
	}

	bool Near(float a, float b) { return std::abs(a - b) < 1e-5f; }

	void TestResolver()
	{
		TestWorld world;
		using Op = sas::AttributeModifierOperation;
		using Target = ly::ModuleAttributeTarget;

		// Offense dot carrying every kind of modifier.
		ly::ModuleSizeVariant& dot = world.modules[2].sizeVariants[0];
		dot.attributeModifiers = {
			Attr(Target::Owner, "Owner.A", Op::Add, 10.f),
			Attr(Target::Ship, "Ship.B", Op::Multiply, 1.12f),
			Attr(Target::PrimaryWeapon, "Weapon.C", Op::Add, 4.f),
			Attr(Target::Owner, "Owner.D", Op::Override, 7.f),
			Attr(Target::Owner, "Owner.E", Op::Add, 10.f, false)
		};
		ly::ModuleAbilityModifier abilityModifier;
		abilityModifier.modifier.attributeId = sas::AttributeId{ "Common.Damage" };
		abilityModifier.modifier.operation = Op::Multiply;
		abilityModifier.modifier.magnitude = 1.12f;
		dot.abilityModifiers = { abilityModifier };
		world.modules[5].sizeVariants[0].attributeModifiers = { Attr(Target::Owner, "Owner.A", Op::Add, 10.f) };

		const auto rules = world.Rules();
		const auto resolveAt = [&](int x, int y)
		{
			return ly::ModuleResolver::Resolve(rules, { MakePlacement("Module.Offense.Matrix.Dot", x, y, 0, 0, 1) });
		};

		// Wings match the Offense family: +25%.
		const auto wings = resolveAt(2, 0);
		Check(wings.ownerModifiers.size() == 3 && wings.shipModifiers.size() == 1 && wings.primaryWeaponModifiers.size() == 1, "routing per target");
		if (wings.ownerModifiers.size() == 3 && wings.shipModifiers.size() == 1 && wings.primaryWeaponModifiers.size() == 1)
		{
			Check(Near(wings.ownerModifiers[0].magnitude, 12.5f), "matching zone scales Add by 1.25");
			Check(Near(wings.shipModifiers[0].magnitude, 1.15f), "matching zone scales Multiply 1.12 to 1.15");
			Check(Near(wings.primaryWeaponModifiers[0].magnitude, 5.f), "matching zone scales weapon Add");
			Check(Near(wings.ownerModifiers[1].magnitude, 7.f), "Override is never scaled");
			Check(Near(wings.ownerModifiers[2].magnitude, 10.f), "affectedByZoneBonus=false is not scaled");
		}
		Check(wings.abilityRules.size() == 1 && wings.abilityRules[0].requiredAbilityTags.size() == 1 &&
			wings.abilityRules[0].requiredAbilityTags[0] == Tags::Ability::Offense, "scoped rule requires the module family");
		Check(wings.abilityRules.size() == 1 && wings.abilityRules[0].attributeModifiers.size() == 1 &&
			Near(wings.abilityRules[0].attributeModifiers[0].magnitude, 1.15f), "ability modifier is scaled");
		Check(wings.enabledSystems.empty(), "a non-system module enables nothing");

		// Hull is a wildcard zone: +10%.
		const auto hull = resolveAt(1, 1);
		Check(hull.ownerModifiers.size() == 3 && Near(hull.ownerModifiers[0].magnitude, 11.f), "Hull wildcard scales Add by 1.10");
		Check(hull.shipModifiers.size() == 1 && Near(hull.shipModifiers[0].magnitude, 1.132f), "Hull wildcard scales Multiply");

		// Nose rewards Primary, not Offense: unchanged.
		const auto nose = resolveAt(0, 0);
		Check(nose.ownerModifiers.size() == 3 && Near(nose.ownerModifiers[0].magnitude, 10.f) &&
			nose.shipModifiers.size() == 1 && Near(nose.shipModifiers[0].magnitude, 1.12f), "non-matching zone leaves values unchanged");

		// Domino across Hull (2,1) and Wings (3,1) spans two zones: no bonus.
		const auto spanning = ly::ModuleResolver::Resolve(rules, { MakePlacement("Module.Offense.Matrix.Domino", 2, 1, 0, 0, 2) });
		Check(spanning.ownerModifiers.size() == 1 && Near(spanning.ownerModifiers[0].magnitude, 10.f), "spanning two zones gets no bonus");
		const auto insideWings = ly::ModuleResolver::Resolve(rules, { MakePlacement("Module.Offense.Matrix.Domino", 2, 0, 0, 0, 2) });
		Check(insideWings.ownerModifiers.size() == 1 && Near(insideWings.ownerModifiers[0].magnitude, 12.5f), "a domino inside Wings gets the bonus");
		const auto rotated = ly::ModuleResolver::Resolve(rules, { MakePlacement("Module.Offense.Matrix.Domino", 3, 0, 1, 0, 2) });
		Check(rotated.ownerModifiers.size() == 1 && Near(rotated.ownerModifiers[0].magnitude, 12.5f), "rotated cells determine the zone");

		// Side stats stay unscaled, output follows placement order, systems are listed.
		ly::ModulePlacement withSides = MakePlacement("Module.Offense.Matrix.Dot", 2, 0, 0, 0, 3);
		withSides.rarity = ly::ModuleRarity::Rare;
		withSides.rolledSideStats = { sas::AttributeModifier{ sas::AttributeId{ "Owner.Side" }, 8.f } };
		const ly::ModulePlacement shield = MakePlacement("Module.Defense.ShieldGenerator.Standard", 0, 2, 0, 0, 4);
		const auto mixed = ly::ModuleResolver::Resolve(rules, { shield, withSides });
		Check(mixed.enabledSystems.size() == 1 && mixed.enabledSystems[0] == Tags::Module::System::Shield, "a placed system module is enabled");
		Check(!mixed.ownerModifiers.empty() && mixed.ownerModifiers.back().attributeId.GetName() == "Owner.Side" &&
			Near(mixed.ownerModifiers.back().magnitude, 8.f), "side stat is unscaled");
		Check(mixed.abilityRules.size() == 1, "a module without ability modifiers emits no rule");

		const auto none = ly::ModuleResolver::Resolve(rules, {});
		Check(none.ownerModifiers.empty() && none.abilityRules.empty() && none.enabledSystems.empty(), "empty placements resolve to nothing");
	}

	// ---- first module content ----

	struct ExpectedEffect
	{
		enum class Kind { Owner, Ship, Primary, Ability };
		Kind kind;
		const char* attribute;
		sas::AttributeModifierOperation operation;
		float rawMagnitude;
		bool scaled;
	};

	struct ExpectedVariant
	{
		const char* id;
		std::size_t variant;
		std::size_t cells;
		// Bonus the best shipped placement must reach; negative accepts the best available.
		float requiredBonus;
		std::vector<ExpectedEffect> effects;
	};

	// Best legal placement of one variant over the given ships, on an empty loadout (so system modules
	// are restricted only to their zone). Returns false when it fits nowhere.
	bool FindBestPlacement(
		const std::vector<const ly::ShipSchematic*>& schematics,
		const char* id,
		std::size_t variant,
		const ly::ShipSchematic*& outSchematic,
		ly::ModulePlacement& outPlacement,
		float& outBonus)
	{
		outBonus = -1.f;
		outSchematic = nullptr;
		const ly::ModuleDefinition* module = ly::content::ModuleContentCatalog::FindById(id);
		if (!module) return false;
		for (const ly::ShipSchematic* schematic : schematics)
		{
			const ly::ModuleRulesContext rules{ *schematic,
				[](const sas::ContentId& moduleId) { return ly::content::ModuleContentCatalog::FindById(moduleId); },
				ly::content::ModuleContentCatalog::GetZoneTable() };
			for (int rotation = 0; rotation < 4; ++rotation)
			for (int y = 0; y < schematic->height; ++y)
			for (int x = 0; x < schematic->width; ++x)
			{
				const ly::ModulePlacement candidate = MakePlacement(id, x, y, rotation, static_cast<int>(variant), 1);
				if (ly::ModulePlacementRules::CanPlace(rules, ly::List<ly::ModulePlacement>{}, candidate) != ModulePlacementFailure::None) continue;
				const float bonus = ly::ModuleResolver::GetZoneBonus(rules, *module, candidate);
				if (bonus > outBonus) { outBonus = bonus; outPlacement = candidate; outSchematic = schematic; }
			}
		}
		return outSchematic != nullptr;
	}

	void TestFirstModuleContent()
	{
		using Kind = ExpectedEffect::Kind;
		using Op = sas::AttributeModifierOperation;
		const auto ships = ly::content::ShipLoader::LoadFromFile(DataPath("ships.json"), MakeBaseShip());
		std::string failure;
		Check(ships.Succeeded() && ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "content module setup failed");
		if (!ships.Succeeded()) return;

		std::vector<const ly::ShipSchematic*> allSchematics;
		std::vector<const ly::ShipSchematic*> fighterSchematic;
		for (const auto& loaded : ships.definitions)
		{
			if (!loaded.definition.schematic) continue;
			allSchematics.push_back(&*loaded.definition.schematic);
			if (loaded.id == "Ship.Player.Fighter.Basic") fighterSchematic.push_back(&*loaded.definition.schematic);
		}

		const std::vector<ExpectedVariant> expected = {
			{ "Module.Primary.FocusedBarrel.Basic", 0, 2, 0.25f, { { Kind::Primary, "Common.Damage", Op::Multiply, 1.10f, true } } },
			{ "Module.Primary.FocusedBarrel.Basic", 1, 3, 0.25f, { { Kind::Primary, "Common.Damage", Op::Multiply, 1.14f, true } } },
			{ "Module.Primary.PiercingTip.Basic", 0, 2, 0.25f, { { Kind::Primary, "PrimaryWeapon.Projectile.Delivery.PierceCount", Op::Add, 1.f, false } } },
			{ "Module.Primary.FireRateBooster.Basic", 0, 2, 0.25f, { { Kind::Primary, "Common.FireRate", Op::Multiply, 1.10f, true } } },
			{ "Module.Primary.FireRateBooster.Basic", 1, 3, 0.25f, { { Kind::Primary, "Common.FireRate", Op::Multiply, 1.14f, true } } },
			{ "Module.Offense.AttackMatrix.Basic", 0, 2, 0.25f, { { Kind::Ability, "Common.Damage", Op::Multiply, 1.12f, true } } },
			{ "Module.Offense.AttackMatrix.Basic", 1, 3, 0.25f, { { Kind::Ability, "Common.Damage", Op::Multiply, 1.16f, true } } },
			// The 2x2 shape fits at least one shipped Wings zone, so it reaches the matching bonus.
			{ "Module.Offense.OverloadCore.Basic", 0, 4, 0.25f, {
				{ Kind::Ability, "Common.Damage", Op::Multiply, 1.25f, true },
				{ Kind::Ability, "Common.Cooldown", Op::Multiply, 1.10f, false } } },
			{ "Module.Defense.ArmorPlate.Basic", 0, 2, 0.25f, { { Kind::Owner, "Owner.Armor", Op::Add, 4.f, true } } },
			{ "Module.Defense.ArmorPlate.Basic", 1, 3, 0.25f, { { Kind::Owner, "Owner.Armor", Op::Add, 5.5f, true } } },
			{ "Module.Defense.HullReinforcement.Basic", 0, 2, 0.25f, { { Kind::Owner, "Owner.MaxHealth", Op::Add, 40.f, true } } },
			{ "Module.Defense.HullReinforcement.Basic", 1, 3, 0.25f, { { Kind::Owner, "Owner.MaxHealth", Op::Add, 55.f, true } } },
			{ "Module.Movement.ThrustBooster.Basic", 0, 2, 0.25f, {
				{ Kind::Owner, "Owner.MoveSpeedHorizontal", Op::Add, 2.2f, true }, { Kind::Owner, "Owner.MoveSpeedVertical", Op::Add, 2.2f, true } } },
			{ "Module.Movement.ThrustBooster.Basic", 1, 3, 0.25f, {
				{ Kind::Owner, "Owner.MoveSpeedHorizontal", Op::Add, 3.f, true }, { Kind::Owner, "Owner.MoveSpeedVertical", Op::Add, 3.f, true } } },
			{ "Module.Movement.ManeuverThruster.Basic", 0, 1, 0.25f, { { Kind::Owner, "Owner.MoveSpeedHorizontal", Op::Add, 0.36f, true } } },
			{ "Module.Utility.CoolingCircuit.Basic", 0, 2, 0.25f, { { Kind::Owner, "Owner.AbilityHaste", Op::Add, 8.f, true } } },
			{ "Module.Utility.CoolingCircuit.Basic", 1, 3, 0.25f, { { Kind::Owner, "Owner.AbilityHaste", Op::Add, 11.f, true } } },
			{ "Module.Utility.EnergyCell.Basic", 0, 2, 0.25f, { { Kind::Owner, "Owner.Energy.Power", Op::Add, 10.f, true } } },
			{ "Module.Utility.EnergyCell.Basic", 1, 3, 0.25f, { { Kind::Owner, "Owner.Energy.Power", Op::Add, 14.f, true } } },
			{ "Module.Utility.TargetingComputer.Basic", 0, 1, 0.25f, { { Kind::Owner, "Owner.CriticalChance", Op::Add, 3.f, true } } },
			{ "Module.Utility.TargetingComputer.Basic", 1, 2, 0.25f, { { Kind::Owner, "Owner.CriticalChance", Op::Add, 5.f, true } } },
			{ "Module.Defense.ShieldGenerator.FastCharge", 0, 2, 0.25f, {
				{ Kind::Ship, "Ship.Shield.Max", Op::Multiply, 0.8f, false }, { Kind::Ship, "Ship.Shield.Regen", Op::Multiply, 1.5f, true } } },
			{ "Module.Defense.ShieldGenerator.Heavy", 0, 3, 0.25f, { { Kind::Ship, "Ship.Shield.Max", Op::Add, 60.f, true } } },
			{ "Module.Movement.Afterburner.Extended", 0, 3, 0.25f, { { Kind::Ship, "Ship.Afterburner.Capacity", Op::Add, 50.f, true } } }
		};

		std::set<std::string> covered;
		for (const ExpectedVariant& entry : expected)
		{
			covered.insert(entry.id);
			const std::string label = std::string(entry.id) + " v" + std::to_string(entry.variant);
			const ly::ModuleDefinition* module = ly::content::ModuleContentCatalog::FindById(entry.id);
			Check(module != nullptr && entry.variant < module->sizeVariants.size(), label + " is missing from the catalog");
			if (!module || entry.variant >= module->sizeVariants.size()) continue;
			Check(module->sizeVariants[entry.variant].shape.cells.size() == entry.cells, label + " cell count");
			Check(!module->displayName.empty() && module->displayName != entry.id, label + " carries a display name");

			const ly::ShipSchematic* schematic = nullptr;
			ly::ModulePlacement best;
			float bestBonus = -1.f;
			Check(FindBestPlacement(allSchematics, entry.id, entry.variant, schematic, best, bestBonus), label + " fits no shipped ship");
			if (!schematic) continue;
			if (entry.requiredBonus >= 0.f) Check(Near(bestBonus, entry.requiredBonus), label + " reaches its matching zone bonus");

			const ly::ModuleRulesContext rules{ *schematic,
				[](const sas::ContentId& moduleId) { return ly::content::ModuleContentCatalog::FindById(moduleId); },
				ly::content::ModuleContentCatalog::GetZoneTable() };
			const ly::ResolvedModuleEffects effects = ly::ModuleResolver::Resolve(rules, { best });
			const auto expectedValue = [&](const ExpectedEffect& effect)
			{
				if (!effect.scaled) return effect.rawMagnitude;
				return effect.operation == sas::AttributeModifierOperation::Multiply
					? 1.f + (effect.rawMagnitude - 1.f) * (1.f + bestBonus) : effect.rawMagnitude * (1.f + bestBonus);
			};
			const auto compare = [&](const ly::List<sas::AttributeModifier>& actual, Kind kind)
			{
				std::vector<const ExpectedEffect*> wanted;
				for (const ExpectedEffect& effect : entry.effects) if (effect.kind == kind) wanted.push_back(&effect);
				Check(actual.size() == wanted.size(), label + " resolved modifier count");
				if (actual.size() != wanted.size()) return;
				for (std::size_t i = 0; i < wanted.size(); ++i)
				{
					Check(actual[i].attributeId.GetName() == wanted[i]->attribute && actual[i].operation == wanted[i]->operation,
						label + " resolved modifier identity " + wanted[i]->attribute);
					Check(Near(actual[i].magnitude, expectedValue(*wanted[i])), label + " resolved magnitude " + wanted[i]->attribute);
				}
			};
			compare(effects.ownerModifiers, Kind::Owner);
			compare(effects.shipModifiers, Kind::Ship);
			compare(effects.primaryWeaponModifiers, Kind::Primary);
			ly::List<sas::AttributeModifier> abilityModifiers;
			for (const ly::ScopedAbilityRule& rule : effects.abilityRules)
			{
				Check(rule.requiredAbilityTags.size() == 1 && rule.requiredAbilityTags[0] == module->family, label + " scoped rule family");
				for (const auto& modifier : rule.attributeModifiers) abilityModifiers.push_back(modifier);
			}
			compare(abilityModifiers, Kind::Ability);
			Check(effects.enabledSystems.size() == (module->systemCategory.name.empty() ? 0u : 1u), label + " enabled systems");
		}

		// Hard-coded final values on Fighter, with the matching zone bonus (0.25) applied.
		const auto resolveOnFighter = [&](const char* id)
		{
			const ly::ShipSchematic* schematic = nullptr;
			ly::ModulePlacement placement;
			float bonus = 0.f;
			FindBestPlacement(fighterSchematic, id, 0, schematic, placement, bonus);
			Check(schematic != nullptr && Near(bonus, 0.25f), std::string(id) + " reaches the matching zone on Fighter");
			if (!schematic) return ly::ResolvedModuleEffects{};
			const ly::ModuleRulesContext rules{ *schematic,
				[](const sas::ContentId& moduleId) { return ly::content::ModuleContentCatalog::FindById(moduleId); },
				ly::content::ModuleContentCatalog::GetZoneTable() };
			return ly::ModuleResolver::Resolve(rules, { placement });
		};
		const auto armor = resolveOnFighter("Module.Defense.ArmorPlate.Basic");
		Check(armor.ownerModifiers.size() == 1 && Near(armor.ownerModifiers[0].magnitude, 5.f), "Armor Plate in the Core resolves to +5 armor");
		const auto heavy = resolveOnFighter("Module.Defense.ShieldGenerator.Heavy");
		Check(heavy.shipModifiers.size() == 1 && Near(heavy.shipModifiers[0].magnitude, 75.f), "Heavy Shield in the Core resolves to +75 shield");
		const auto fast = resolveOnFighter("Module.Defense.ShieldGenerator.FastCharge");
		Check(fast.shipModifiers.size() == 2 && Near(fast.shipModifiers[0].magnitude, 0.8f) && Near(fast.shipModifiers[1].magnitude, 1.625f),
			"Fast Charge resolves to x0.8 max shield (no zone bonus) and x1.625 regen");
		const auto barrel = resolveOnFighter("Module.Primary.FocusedBarrel.Basic");
		Check(barrel.primaryWeaponModifiers.size() == 1 && Near(barrel.primaryWeaponModifiers[0].magnitude, 1.125f), "Focused Barrel in the Nose resolves to x1.125 damage");

		// Every module is either one of the four standard system modules or covered above.
		std::size_t standardCount = 0;
		for (const ly::ModuleDefinition& definition : ly::content::ModuleContentCatalog::GetDefinitions())
		{
			const std::string id = definition.moduleId.ToString();
			if (id.size() >= 9 && id.compare(id.size() - 9, 9, ".Standard") == 0) { ++standardCount; continue; }
			Check(covered.count(id) == 1, "content module has no test expectation: " + id);
		}
		Check(standardCount == 4, "four standard modules");
	}

	// Shape matters, not whole module classes: with the default modules installed, every free pair of adjacent
	// cells in a zone holds a 2-cell module, judged by the real placement rules. Schematics are mirror-symmetric,
	// so zones may split into mirrored left and right parts and are not required to be one connected region.
	void TestShippedZoneShapeFit()
	{
		const auto ships = ly::content::ShipLoader::LoadFromFile(DataPath("ships.json"), MakeBaseShip());
		std::string failure;
		Check(ships.Succeeded() && ly::content::ModuleContentCatalog::LoadFromFile(DataPath("modules.json"), &failure), "zone shape fit setup failed");
		if (!ships.Succeeded()) return;

		const char* dominoId = "Module.Offense.AssaultCore.Basic";
		const ly::ModuleDefinition* domino = ly::content::ModuleContentCatalog::FindById(dominoId);
		Check(domino && domino->sizeVariants.size() > 0 && domino->sizeVariants[0].shape.cells.size() == 2, "domino probe module is a two cell shape");
		if (!domino) return;

		const ly::GameplayTag zones[] = {
			ly::GameplayTags::Module::Zone::Nose, ly::GameplayTags::Module::Zone::Wings, ly::GameplayTags::Module::Zone::Hull,
			ly::GameplayTags::Module::Zone::Core, ly::GameplayTags::Module::Zone::Engine, ly::GameplayTags::Module::Zone::Sensor
		};

		int shipCount = 0;
		for (const auto& loaded : ships.definitions)
		{
			if (!loaded.definition.schematic) continue;
			++shipCount;
			const ly::ShipSchematic& schematic = *loaded.definition.schematic;
			const ly::ModuleRulesContext rules{ schematic,
				[](const sas::ContentId& moduleId) { return ly::content::ModuleContentCatalog::FindById(moduleId); },
				ly::content::ModuleContentCatalog::GetZoneTable() };

			ly::List<ly::ModulePlacement> installed;
			std::set<std::pair<int, int>> occupied;
			std::uint32_t nextId = 1;
			for (const ly::DefaultModulePlacement& item : schematic.defaultModules)
			{
				ly::ModulePlacement placement = MakePlacement(item.moduleId.ToString().c_str(), item.origin.x, item.origin.y,
					item.rotationQuarterTurns, item.variantIndex, nextId++);
				const ly::ModuleDefinition* module = ly::content::ModuleContentCatalog::FindById(item.moduleId);
				if (!module) continue;
				for (const sf::Vector2i& cell : ly::ModulePlacementRules::GetCoveredCells(*module, placement)) occupied.insert({ cell.x, cell.y });
				installed.push_back(std::move(placement));
			}

			for (const ly::GameplayTag& zone : zones)
			{
				// Free cells of the whole zone.
				std::set<std::pair<int, int>> zoneCells;
				int freeCount = 0;
				for (int y = 0; y < schematic.height; ++y)
				for (int x = 0; x < schematic.width; ++x)
				{
					if (schematic.GetZoneAt(x, y) != zone) continue;
					zoneCells.insert({ x, y });
					if (occupied.count({ x, y }) == 0) ++freeCount;
				}
				const std::string label = loaded.id + " " + zone.name;
				bool freePairExists = false;
				for (const std::pair<int, int>& cell : zoneCells)
					for (const std::pair<int, int>& next : { std::pair<int, int>{ cell.first + 1, cell.second }, { cell.first, cell.second + 1 } })
						if (zoneCells.count(next) != 0 && occupied.count(cell) == 0 && occupied.count(next) == 0) freePairExists = true;

				const auto fitsAnywhere = [&](const ly::ModuleDefinition& module, int variant, const char* id)
				{
					for (int rotation = 0; rotation < 4; ++rotation)
					for (int y = 0; y < schematic.height; ++y)
					for (int x = 0; x < schematic.width; ++x)
					{
						const ly::ModulePlacement candidate = MakePlacement(id, x, y, rotation, variant, nextId);
						if (ly::ModulePlacementRules::CanPlace(rules, installed, candidate) != ModulePlacementFailure::None) continue;
						bool inside = true;
						for (const sf::Vector2i& cell : ly::ModulePlacementRules::GetCoveredCells(module, candidate))
						{
							if (schematic.GetZoneAt(cell.x, cell.y) != zone) inside = false;
						}
						if (inside) return true;
					}
					return false;
				};
				if (freePairExists) Check(fitsAnywhere(*domino, 0, dominoId), label + " (" + std::to_string(freeCount) + " free) fits a 2-cell module");
			}
		}
		Check(shipCount == 6, "zone shape fit covers the six player ships");
	}

	void TestRotation()
	{
		using ly::ModuleShapeRules::Rotate;
		const auto asSet = [](const ly::List<sf::Vector2i>& cells)
		{
			std::set<std::pair<int, int>> result;
			for (const sf::Vector2i& cell : cells) result.insert({ cell.x, cell.y });
			return result;
		};
		using Cells = std::set<std::pair<int, int>>;

		const ly::List<sf::Vector2i> domino{ { 0, 0 }, { 1, 0 } };
		Check(asSet(Rotate(domino, 0)) == Cells{ { 0, 0 }, { 1, 0 } }, "domino 0 turns");
		Check(asSet(Rotate(domino, 1)) == Cells{ { 0, 0 }, { 0, 1 } }, "domino 1 turn");
		Check(asSet(Rotate(domino, 2)) == Cells{ { 0, 0 }, { 1, 0 } }, "domino 2 turns");
		Check(asSet(Rotate(domino, 3)) == Cells{ { 0, 0 }, { 0, 1 } }, "domino 3 turns");
		Check(asSet(Rotate(domino, 4)) == asSet(domino) && asSet(Rotate(domino, -1)) == asSet(Rotate(domino, 3)), "turns wrap modulo 4");

		const ly::List<sf::Vector2i> lShape{ { 0, 0 }, { 1, 0 }, { 1, 1 } };
		Check(asSet(Rotate(lShape, 0)) == Cells{ { 0, 0 }, { 1, 0 }, { 1, 1 } }, "L 0 turns");
		Check(asSet(Rotate(lShape, 1)) == Cells{ { 1, 0 }, { 1, 1 }, { 0, 1 } }, "L 1 turn");
		Check(asSet(Rotate(lShape, 2)) == Cells{ { 1, 1 }, { 0, 1 }, { 0, 0 } }, "L 2 turns");
		Check(asSet(Rotate(lShape, 3)) == Cells{ { 0, 1 }, { 0, 0 }, { 1, 0 } }, "L 3 turns");

		for (const auto& cell : Rotate(lShape, 1))
		{
			Check(cell.x >= 0 && cell.y >= 0, "rotation must normalize to non-negative offsets");
		}
		const ly::List<sf::Vector2i> single{ { 0, 0 } };
		Check(asSet(Rotate(single, 2)) == Cells{ { 0, 0 } }, "single cell is rotation invariant");
		Check(Rotate({}, 1).empty(), "empty shape stays empty");
	}
}

int main()
{
	TestIdentities();
	TestShippedModules();
	TestModuleRejections();
	TestShippedSchematics();
	TestShipLoaderRejections();
	TestSchematicValidatorRejections();
	TestRotation();
	TestCanPlace();
	TestDuplicateSystem();
	TestLoadoutMutations();
	TestDefaultSeeding();
	TestResolver();
	TestFirstModuleContent();
	TestShippedZoneShapeFit();

	if (gFailures != 0)
	{
		std::cerr << gFailures << " module system check(s) failed\n";
		return 1;
	}
	std::cout << "Module system tests passed\n";
	return 0;
}
