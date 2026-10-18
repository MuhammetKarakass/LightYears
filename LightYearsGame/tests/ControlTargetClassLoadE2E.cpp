#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameConfigs/ship/ShipConfig.h"
#include "gameplay/content/ShipLoader.h"

#include <map>
#include <string>

namespace control_target_class_load_e2e
{
	using namespace vega;
	using namespace common_module_e2e;
	namespace
	{
		struct ClassCase
		{
			const char* name;
			ControlTargetClass expected;
		};

		struct InvalidCase
		{
			const char* name;
			Json value;
			const char* expectedText;
		};

		const char* ClassName(ControlTargetClass value)
		{
			switch (value)
			{
			case ControlTargetClass::Normal: return "Normal";
			case ControlTargetClass::Elite: return "Elite";
			case ControlTargetClass::MiniBoss: return "MiniBoss";
			case ControlTargetClass::Boss: return "Boss";
			}
			return "Unknown";
		}

		std::filesystem::path WriteShipFixture(const std::filesystem::path& root, const std::string& name, const Json& ship)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / (name + ".json");
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write control target class fixture" };
			output << Json{ { "schemaVersion", 1 }, { "ships", Json::array({ ship }) } }.dump(2) << '\n';
			return path;
		}

		content::ShipLoader::Result LoadFixture(const std::filesystem::path& path)
		{
			return content::ShipLoader::LoadFromFile(path, ShipData::Ship_Player_Fighter);
		}
	}

	void LoadChecks(Report& report, Application& application, const std::filesystem::path& fixtureRoot)
	{
		const std::filesystem::path shippedPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/ships.json";
		std::ifstream shippedInput{ shippedPath };
		report.Check(static_cast<bool>(shippedInput), "Shipped ships.json can be opened");
		const Json shipped = Json::parse(shippedInput);

		Json enemyBase;
		for (const Json& ship : shipped.at("ships"))
		{
			if (enemyBase.is_null() && ship.at("id").get<std::string>().rfind("Ship.Enemy.", 0) == 0) enemyBase = ship;
		}
		report.Check(!enemyBase.is_null(), "Shipped ships.json provides an enemy chassis to clone");

		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();

		const ClassCase classes[] = {
			{ "Normal", ControlTargetClass::Normal },
			{ "Elite", ControlTargetClass::Elite },
			{ "MiniBoss", ControlTargetClass::MiniBoss },
			{ "Boss", ControlTargetClass::Boss }
		};
		for (const ClassCase& item : classes)
		{
			Json ship = enemyBase;
			ship["controlTargetClass"] = item.name;
			const content::ShipLoader::Result loaded = LoadFixture(WriteShipFixture(fixtureRoot, std::string{ "class-" } + item.name, ship));
			report.Check(loaded.Succeeded() && loaded.definitions.size() == 1, std::string{ "Fixture with controlTargetClass " } + item.name + " loads one ship", Json{ { "input", item.name }, { "error", loaded.error } });
			const auto spaceShip = level->SpawnActor<SpaceShip>(loaded.definitions.front().definition).lock();
			level->TickInternal(0.f);
			report.Check(spaceShip != nullptr, std::string{ "Parsed " } + item.name + " spawns a real SpaceShip");
			report.Check(spaceShip->GetControlTargetClass() == item.expected, std::string{ "Real SpaceShip reports " } + ClassName(item.expected), Json{ { "input", item.name }, { "result", ClassName(spaceShip->GetControlTargetClass()) } });
		}

		{
			Json ship = enemyBase;
			ship.erase("controlTargetClass");
			const content::ShipLoader::Result loaded = LoadFixture(WriteShipFixture(fixtureRoot, "class-absent", ship));
			report.Check(loaded.Succeeded() && loaded.definitions.size() == 1, "Fixture without controlTargetClass loads one ship", Json{ { "input", "absent" }, { "error", loaded.error } });
			const auto spaceShip = level->SpawnActor<SpaceShip>(loaded.definitions.front().definition).lock();
			level->TickInternal(0.f);
			report.Check(spaceShip != nullptr && spaceShip->GetControlTargetClass() == ControlTargetClass::Normal, "Missing controlTargetClass defaults to Normal on a real SpaceShip", Json{ { "input", "absent" }, { "result", spaceShip ? ClassName(spaceShip->GetControlTargetClass()) : "none" } });
		}

		const InvalidCase invalidCases[] = {
			{ "lowercase", "elite", "Unknown ship controlTargetClass: elite" },
			{ "unknown", "Champion", "Unknown ship controlTargetClass: Champion" },
			{ "number", 3, "Ship controlTargetClass must be a string" }
		};
		for (const InvalidCase& item : invalidCases)
		{
			Json ship = enemyBase;
			ship["controlTargetClass"] = item.value;
			const content::ShipLoader::Result loaded = LoadFixture(WriteShipFixture(fixtureRoot, std::string{ "invalid-" } + item.name, ship));
			report.Check(!loaded.Succeeded() && loaded.error.find(item.expectedText) != std::string::npos, std::string{ "Invalid controlTargetClass is rejected: " } + item.name, Json{ { "input", item.value }, { "error", loaded.error } });
		}

		const content::ShipLoader::Result shippedResult = LoadFixture(shippedPath);
		report.Check(shippedResult.Succeeded(), "Shipped ships.json loads through the ship parser", Json{ { "error", shippedResult.error } });
		std::map<std::string, std::string> declaredClasses;
		for (const Json& ship : shipped.at("ships")) declaredClasses[ship.at("id").get<std::string>()] = ship.value("controlTargetClass", std::string{ "Normal" });
		Json shippedClasses = Json::array();
		bool allMatch = !shippedResult.definitions.empty();
		for (const auto& loadedShip : shippedResult.definitions)
		{
			const std::string parsedClass = ClassName(loadedShip.definition.controlTargetClass);
			const auto declaredIt = declaredClasses.find(loadedShip.id);
			allMatch = allMatch && declaredIt != declaredClasses.end() && declaredIt->second == parsedClass;
			shippedClasses.push_back(loadedShip.id + "=" + parsedClass);
		}
		report.Check(allMatch, "Every shipped ship parses to the class its JSON declares (Normal when absent)", Json{ { "shipCount", shippedResult.definitions.size() }, { "classes", shippedClasses } });
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace control_target_class_load_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Control target class content registration failed" };
		std::string title = "Control Target Class Load E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		LoadChecks(report, application, std::filesystem::path{ argv[1] }.parent_path() / "control-target-class-fixtures");
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "ControlTargetClassLoadE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
