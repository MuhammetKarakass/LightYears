#include "framework/JsonDocumentLoader.h"
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipLoader.h"
#include "gameplay/module/ShipSchematicValidator.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>

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

		Check(result.definitions.size() == 4, "shipped modules.json should hold the four standard system modules");
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
			{ "Ship.Player.Fighter.Basic", 4, 5, 7, 5, 4, 3 },
			{ "Ship.Player.Breacher.Basic", 7, 6, 6, 4, 2, 3 },
			{ "Ship.Player.Interceptor.Basic", 3, 4, 6, 4, 9, 2 },
			{ "Ship.Player.Conductor.Basic", 2, 4, 7, 5, 3, 7 },
			{ "Ship.Player.Aegis.Basic", 2, 3, 4, 12, 4, 3 },
			{ "Ship.Player.CryoController.Basic", 3, 4, 6, 4, 4, 7 }
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
			Check(total == ly::ShipSchematicValidator::kFreeCellCapacity, std::string(expected.id) + " free cell total is not 28");
			Check(schematic.width >= 6 && schematic.width <= 8, std::string(expected.id) + " grid width out of 6..8");
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
		expectRejected("shield outside Core", [](ly::ShipSchematic& s) { s.defaultModules[0].origin = sf::Vector2i{ 2, 6 }; });
		expectRejected("out of bounds", [](ly::ShipSchematic& s) { s.defaultModules[1].origin = sf::Vector2i{ 6, 7 }; });
		expectRejected("negative origin", [](ly::ShipSchematic& s) { s.defaultModules[1].origin = sf::Vector2i{ -1, 7 }; });
		expectRejected("overlap", [](ly::ShipSchematic& s) { s.defaultModules[3].origin = s.defaultModules[2].origin; });
		expectRejected("duplicate system", [](ly::ShipSchematic& s) { s.defaultModules[3].moduleId = s.defaultModules[2].moduleId; s.defaultModules[3].origin = sf::Vector2i{ 3, 2 }; });
		expectRejected("rotation leaves zone", [](ly::ShipSchematic& s) { s.defaultModules[1].rotationQuarterTurns = 1; });
		expectRejected("27 free cells", [](ly::ShipSchematic& s) { s.cellZones[static_cast<std::size_t>(0 * s.width + 2)] = ly::GameplayTag{}; });
		expectRejected("29 free cells", [](ly::ShipSchematic& s) { s.cellZones[0] = ly::GameplayTags::Module::Zone::Nose; });
		expectRejected("unknown zone in grid", [](ly::ShipSchematic& s) { s.cellZones[2] = ly::GameplayTag{ "Module.Zone.Cockpit" }; });
		expectRejected("grid size mismatch", [](ly::ShipSchematic& s) { s.cellZones.pop_back(); });

		// A rotated (vertical) shield domino fits when the Core has room below.
		ly::ShipSchematic vertical = fighter;
		vertical.defaultModules[0].origin = sf::Vector2i{ 3, 4 };
		vertical.defaultModules[0].rotationQuarterTurns = 1;
		Check(validate(vertical), "a rotated default module that fits the Core should validate");
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

	if (gFailures != 0)
	{
		std::cerr << gFailures << " module system check(s) failed\n";
		return 1;
	}
	std::cout << "Module system tests passed\n";
	return 0;
}
