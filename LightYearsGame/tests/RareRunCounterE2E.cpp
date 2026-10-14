#include "UncommonModuleP4E2E.h"

#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace rare_run_counter_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kCounterId = "Module.Movement.RareRunCounterFixture.Rare";
		constexpr const char* kPlainId = "Module.Movement.RareRunCounterPlainFixture.Rare";
		constexpr float kDriveSpeed = 400.f;
		constexpr float kDriveStep = 0.1f;

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		ly::List<ly::ModuleDefinition> gFixtureDefinitions;

		const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
		{
			for (const ModuleDefinition& definition : gFixtureDefinitions)
				if (definition.moduleId == id) return &definition;
			return content::ModuleContentCatalog::FindById(id);
		}

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		// Level 1 is 1% per stack, level 2 1.2% and level 3 1.4%, for both damage and movement speed.
		Json StandardCounterRules()
		{
			return Json::array({ Json{ { "source", "Distance" }, { "unitsPerStack", 5000 },
				{ "damagePerStack", Json::array({ 0.01, 0.012, 0.014 }) }, { "moveSpeedPerStack", Json::array({ 0.01, 0.012, 0.014 }) } } });
		}

		Json RareModule(const char* id, Json variantFields)
		{
			Json variant{ { "shape", Json::array({ Json::array({ 0, 0 }) }) } };
			for (const auto& field : variantFields.items()) variant[field.key()] = field.value();
			return Json{
				{ "id", id }, { "displayName", "Rare run counter fixture" }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Movement" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ std::move(variant) }) }
			};
		}

		// Shipped modules plus a counter fixture and a plain fixture that has no run counter but a real effect.
		Json FixtureSet(const Json& shipped, const Json& counterRules)
		{
			Json modules = shipped;
			modules["modules"].push_back(RareModule(kCounterId, Json{ { "runCounterRules", counterRules } }));
			modules["modules"].push_back(RareModule(kPlainId, Json{ { "shieldRules", Json::array({ Json{ { "armorShare", 0.5 } } }) } }));
			return modules;
		}

		bool Near(float actual, float expected, float tolerance = 1e-4f)
		{
			return std::abs(actual - expected) <= tolerance;
		}

		float Multiplier(const SpaceShip& ship)
		{
			return ship.GetCombatRuntime().GetOutgoingDamageMultiplier();
		}

		float SpeedMultiplier(const SpaceShip& ship)
		{
			return ship.GetRuntimeModifiers().GetMovementSpeedMultiplier();
		}

		float Speed(const SpaceShip& ship)
		{
			const sf::Vector2f velocity = ship.GetVelocity();
			return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
		}

		// Installs a loadout through the same accepted-mutation path the Player uses, then re-seats any legal origin.
		bool Reinstall(Fixture& f, std::uint32_t id)
		{
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
						if (f.loadout.Install(f.rules, id, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None)
							return f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
			return false;
		}

		bool Relocate(Fixture& f, std::uint32_t id)
		{
			const ModulePlacement* current = f.loadout.FindPlacement(id);
			if (!current) return false;
			const sf::Vector2i from = current->origin;
			const int fromRotation = current->rotationQuarterTurns;
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						if (x == from.x && y == from.y && rotation == fromRotation) continue;
						if (f.loadout.Move(f.rules, id, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None)
							return f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
					}
			return false;
		}

		void Accrue(Fixture& f, double units)
		{
			f.loadout.AccrueRunCounters(f.rules, units);
			f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
		}

		// Damage through the real pipeline from the player ship to a fresh, unshielded, unarmored enemy.
		float HitDamage(Fixture& f)
		{
			const auto target = Enemy(f);
			return Hit(*target, f.ship.get(), 100.f, Plain());
		}

		// Speed after one tick at a commanded speed far above the cap, so the cap alone decides it.
		float CappedSpeed(Fixture& f)
		{
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			f.ship->SetVelocity({ 100000.f, 0.f });
			f.level->TickInternal(kDriveStep);
			return Speed(*f.ship);
		}

		void ExactChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const std::uint32_t id = f.Install(kCounterId, 0, 1);
			const float base = Multiplier(*f.ship);
			const float baseSpeed = SpeedMultiplier(*f.ship);
			const float hitBase = HitDamage(f);
			report.Check(f.loadout.GetRunCounter(id) == 0.0 && Near(Multiplier(*f.ship), base) && Near(SpeedMultiplier(*f.ship), baseSpeed),
				"a fresh odometer starts at stack 0 with no effect");

			Accrue(f, 4999.0);
			report.Check(f.loadout.GetRunCounter(id) == 4999.0 && Near(Multiplier(*f.ship) / base, 1.f) && Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.f),
				"4.999 units keep stack 0: no damage or speed bonus");

			Accrue(f, 1.0);
			const float hitStack1 = HitDamage(f);
			report.Check(f.loadout.GetRunCounter(id) == 5000.0 && Near(Multiplier(*f.ship) / base, 1.01f) &&
				Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.01f) && Near(hitStack1 / hitBase, 1.01f, 1e-3f),
				"5.000 units grant stack 1: +1% outgoing damage and +1% movement speed cap");

			Accrue(f, 5000.0);
			report.Check(f.loadout.GetRunCounter(id) == 10000.0 && Near(Multiplier(*f.ship) / base, 1.02f) && Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.02f),
				"10.000 units grant stack 2");

			Accrue(f, 40000.0);
			report.Check(f.loadout.GetRunCounter(id) == 50000.0 && Near(Multiplier(*f.ship) / base, 1.10f) && Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.10f),
				"no upper limit: 50.000 units grant stack 10");

			const std::size_t installedHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			f.Remove(id);
			report.Check(Near(Multiplier(*f.ship) / base, 1.f) && Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.f),
				"uninstalling removes the stack effects at once");
			report.Check(f.loadout.GetRunCounter(id) == 50000.0, "uninstalling keeps the run counter in the module");
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() + 1 == installedHandles,
				"the stack source leaves the ship with the module");
			Accrue(f, 1000.0);
			report.Check(f.loadout.GetRunCounter(id) == 50000.0, "distance is not counted while the module is uninstalled");

			report.Check(Reinstall(f, id) && f.loadout.GetRunCounter(id) == 50000.0 && Near(Multiplier(*f.ship) / base, 1.10f) &&
				Near(SpeedMultiplier(*f.ship) / baseSpeed, 1.10f),
				"reinstalling restores stack 10 at once from the kept counter");
			const std::size_t reinstalledHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				f.loadout.Uninstall(id);
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
				Reinstall(f, id);
			}
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == reinstalledHandles && f.loadout.GetRunCounter(id) == 50000.0 &&
				Near(Multiplier(*f.ship) / base, 1.10f),
				"ten uninstall and reinstall cycles keep the handle count and the stacks");

			report.Check(Relocate(f, id) && f.loadout.GetRunCounter(id) == 50000.0 && Near(Multiplier(*f.ship) / base, 1.10f),
				"moving the module keeps its run counter and stacks");
		}

		void LevelChecks(Report& report, Application& application, int level, float perStack)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			const float base = Multiplier(*f.ship);
			const float baseSpeed = SpeedMultiplier(*f.ship);
			const std::uint32_t id = f.Install(kCounterId, 0, level);
			Accrue(f, 10000.0);
			const float expected = 1.f + perStack * 2.f;
			report.Check(Near(Multiplier(*f.ship) / base, expected) && Near(SpeedMultiplier(*f.ship) / baseSpeed, expected),
				"level " + std::to_string(level) + " stack 2 uses its own per-stack value",
				Json{ { "expected", expected }, { "damage", Multiplier(*f.ship) / base }, { "counter", f.loadout.GetRunCounter(id) } });
		}

		void SpeedCapChecks(Report& report, Application& application)
		{
			Fixture plain{ report, application };
			plain.rules.lookup = FixtureLookup;
			const float capWithoutModule = CappedSpeed(plain);

			Fixture odometer{ report, application };
			odometer.rules.lookup = FixtureLookup;
			odometer.Install(kCounterId, 0, 1);
			Accrue(odometer, 10000.0);
			const float capStack2 = CappedSpeed(odometer);
			report.Check(capWithoutModule > 0.f && Near(capStack2 / capWithoutModule, 1.02f, 1e-3f),
				"the speed cap rises by the stack multiplier, measured on a clamped ship",
				Json{ { "withoutModule", capWithoutModule }, { "stackTwo", capStack2 } });
		}

		// Installs through the Player's real mutation path, which applies to the live ship.
		bool InstallOnPlayer(Player& player, const ModuleRulesContext& rules, std::uint32_t id)
		{
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
						if (player.TryInstallModule(id, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None) return true;
			return false;
		}

		bool RelocateOnPlayer(Player& player, const ModuleRulesContext& rules, std::uint32_t id)
		{
			const ModulePlacement* current = player.GetModuleLoadout().FindPlacement(id);
			if (!current) return false;
			const sf::Vector2i from = current->origin;
			const int fromRotation = current->rotationQuarterTurns;
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
					{
						if (x == from.x && y == from.y && rotation == fromRotation) continue;
						if (player.TryMoveModule(id, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None) return true;
					}
			return false;
		}

		// One tick at a commanded speed. Returns the distance the ship covered, from its velocity after the tick.
		double Step(HeadlessModuleLevel& level, PlayerSpaceShip& ship, float speed)
		{
			ship.SetVelocity({ speed, 0.f });
			level.TickInternal(kDriveStep);
			const sf::Vector2f velocity = ship.GetVelocity();
			return std::sqrt(static_cast<double>(velocity.x) * velocity.x + static_cast<double>(velocity.y) * velocity.y) * kDriveStep;
		}

		void PlayerChecks(Report& report, Application& application)
		{
			auto level = std::make_shared<HeadlessModuleLevel>(&application);
			level->BeginPlayInternal();
			Player player;
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(ship != nullptr, "the run counter player spawns a real ship through Player::SpawnSpaceShip");
			level->TickInternal(0.f);
			ship->SetUseScreenClamp(false);
			ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const std::optional<ModuleRulesContext> rules = player.BuildModuleRules();
			report.Check(rules.has_value(), "the player ship has a module schematic");

			const std::uint32_t id = player.AddModuleToInventory(kCounterId, 0, ModuleRarity::Rare, 1);
			report.Check(id != 0 && InstallOnPlayer(player, *rules, id), "the counter module installs on the live player loadout");
			const float base = Multiplier(*ship);
			const float baseSpeed = SpeedMultiplier(*ship);
			const auto counter = [&player, id]() { return player.GetModuleLoadout().GetRunCounter(id); };

			// Drive until stack 10. Every tick, the stacks in effect must equal floor(counter / 5000).
			double expected = 0.0;
			int mismatches = 0;
			double firstStackOne = -1.0;
			const sf::Vector2f start = ship->GetActorLocation();
			for (int tick = 0; tick < 4000 && counter() < 50000.0; ++tick)
			{
				expected += Step(*level, *ship, kDriveSpeed);
				const double current = counter();
				const int stack = static_cast<int>(current / 5000.0);
				if (!Near(Multiplier(*ship) / base, 1.f + 0.01f * static_cast<float>(stack), 1e-4f)) ++mismatches;
				if (firstStackOne < 0.0 && stack >= 1) firstStackOne = current;
			}
			const double travelled = static_cast<double>(ship->GetActorLocation().x - start.x);
			report.Check(mismatches == 0 && firstStackOne >= 5000.0 && firstStackOne < 5000.0 + 40.0,
				"driving accrues distance and the stacks follow floor(counter / 5000) on every tick",
				Json{ { "mismatches", mismatches }, { "firstStackOneCounter", firstStackOne } });
			report.Check(std::abs(counter() - expected) <= 1e-3 * expected && std::abs(travelled - expected) <= 0.02 * expected + 1.0,
				"the counter matches the distance the ship really covered",
				Json{ { "counter", counter() }, { "expected", expected }, { "positionDelta", travelled } });
			report.Check(counter() >= 50000.0 && Near(Multiplier(*ship) / base, 1.10f, 1e-3f) && Near(SpeedMultiplier(*ship) / baseSpeed, 1.10f, 1e-3f),
				"the player's stack 10 applies with no upper limit");

			// Uninstalled: effects go away, the counter stays, and driving adds nothing.
			const double atStack10 = counter();
			report.Check(player.TryUninstallModule(id), "the counter module uninstalls through the Player");
			report.Check(Near(Multiplier(*ship) / base, 1.f) && Near(SpeedMultiplier(*ship) / baseSpeed, 1.f) && counter() == atStack10,
				"uninstalling clears the stacks and keeps the counter");
			const sf::Vector2f offStart = ship->GetActorLocation();
			for (int tick = 0; tick < 200; ++tick) Step(*level, *ship, kDriveSpeed);
			report.Check(counter() == atStack10 && ship->GetActorLocation().x - offStart.x > 1.f,
				"the ship keeps moving while uninstalled and the counter does not grow");

			// Reinstalled: stacks return at once from the kept counter, and driving resumes accrual.
			report.Check(InstallOnPlayer(player, *rules, id) && Near(Multiplier(*ship) / base, 1.10f, 1e-3f) && counter() == atStack10,
				"reinstalling restores stack 10 at once with the kept counter");
			double resumed = 0.0;
			for (int tick = 0; tick < 100; ++tick) resumed += Step(*level, *ship, kDriveSpeed);
			report.Check(std::abs(counter() - (atStack10 + resumed)) <= 1e-3 * (atStack10 + resumed),
				"driving after reinstalling adds the covered distance");

			// Moving keeps the counter.
			const double beforeMove = counter();
			report.Check(RelocateOnPlayer(player, *rules, id) && counter() == beforeMove && Near(Multiplier(*ship) / base, 1.10f, 1e-3f),
				"moving the module on the loadout keeps the counter and stacks");

			// Teleports are not travel: no velocity, so the displacement adds nothing.
			ship->SetVelocity({ 0.f, 0.f });
			ship->SetActorLocation(ship->GetActorLocation() + sf::Vector2f{ 10000.f, 0.f });
			level->TickInternal(kDriveStep);
			report.Check(counter() == beforeMove, "a teleport does not add distance to the counter");

			// Handle counts are stable over ten install and uninstall cycles.
			const ModulePlacement* placed = player.GetModuleLoadout().FindPlacement(id);
			const sf::Vector2i origin = placed->origin;
			const int rotation = placed->rotationQuarterTurns;
			const std::size_t handles = ship->GetModuleApplier().GetOwnedHandleCount();
			bool cyclesOk = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				cyclesOk = player.TryUninstallModule(id) && cyclesOk;
				cyclesOk = player.TryInstallModule(id, origin, rotation) == ModulePlacementFailure::None && cyclesOk;
			}
			report.Check(cyclesOk && ship->GetModuleApplier().GetOwnedHandleCount() == handles && counter() == beforeMove,
				"ten Player install and uninstall cycles keep the handle count and the counter");

			// A module without run counter rules neither accrues nor changes the run counter's ship.
			const std::size_t handlesBeforePlain = ship->GetModuleApplier().GetOwnedHandleCount();
			const std::uint32_t plain = player.AddModuleToInventory(kPlainId, 0, ModuleRarity::Rare, 1);
			report.Check(plain != 0 && InstallOnPlayer(player, *rules, plain), "a plain fixture module installs beside the counter");
			// The counter module keeps accruing while driving, so the expected damage is the stack the counter has now.
			for (int tick = 0; tick < 100; ++tick) Step(*level, *ship, kDriveSpeed);
			const int stackNow = static_cast<int>(counter() / 5000.0);
			report.Check(player.GetModuleLoadout().GetRunCounter(plain) == 0.0 && Near(Multiplier(*ship) / base, 1.f + 0.01f * static_cast<float>(stackNow), 1e-3f) &&
				ship->GetModuleApplier().GetOwnedHandleCount() == handlesBeforePlain,
				"a module without run counter rules keeps its counter at zero and adds no stack source",
				Json{ { "stack", stackNow }, { "damage", Multiplier(*ship) / base } });

			// A new run resets the loadout. The player spawns after 3 lives are used up, and the 4th spawn starts a fresh run.
			for (int attempt = 0; attempt < 3; ++attempt) player.SpawnSpaceShip(level.get());
			report.Check(player.GetModuleLoadout().GetRevision() == 0 && player.GetModuleLoadout().FindPlacement(id) == nullptr &&
				player.GetModuleLoadout().GetRunCounter(id) == 0.0,
				"a new run resets the loadout and every run counter with it");
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", FixtureSet(shipped, rules));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "runCounterRules loader rejects " } + name, loaded.error);
			};
			const Json distance{ { "source", "Distance" } };
			const auto withField = [&distance](const char* key, Json value)
			{
				Json rule = distance;
				rule["unitsPerStack"] = 5000;
				rule["damagePerStack"] = 0.01;
				rule[key] = std::move(value);
				return Json::array({ rule });
			};
			reject("unknown-source", Json::array({ Json{ { "source", "Kills" }, { "unitsPerStack", 5000 }, { "damagePerStack", 0.01 } } }));
			reject("missing-source", Json::array({ Json{ { "unitsPerStack", 5000 }, { "damagePerStack", 0.01 } } }));
			reject("missing-units", Json::array({ Json{ { "source", "Distance" }, { "damagePerStack", 0.01 } } }));
			reject("zero-units", withField("unitsPerStack", 0));
			reject("negative-units", withField("unitsPerStack", -5000));
			reject("array-units", withField("unitsPerStack", Json::array({ 5000, 6000 })));
			reject("unknown-field", withField("bonus", 1));
			reject("no-effect", Json::array({ Json{ { "source", "Distance" }, { "unitsPerStack", 5000 } } }));
			reject("zero-effect", Json::array({ Json{ { "source", "Distance" }, { "unitsPerStack", 5000 }, { "damagePerStack", 0 }, { "moveSpeedPerStack", 0 } } }));
			reject("negative-damage", withField("damagePerStack", -0.01));
			reject("negative-table-entry", withField("damagePerStack", Json::array({ 0.01, -0.02, 0.03 })));
			reject("short-table", withField("damagePerStack", Json::array({ 0.01, 0.02 })));
			reject("rules-not-array", Json::object());
			reject("entry-not-object", Json::array({ 1.0 }));

			const auto accepted = WriteJson(root, "accept-scalar.json", FixtureSet(shipped, Json::array({ Json{ { "source", "Distance" },
				{ "unitsPerStack", 5000 }, { "moveSpeedPerStack", 0.01 } } })));
			const auto loaded = content::ModuleLoader::LoadFromFile(accepted);
			report.Check(loaded.Succeeded(), "a scalar run counter rule with only moveSpeedPerStack loads", loaded.error);
			const auto found = std::find_if(loaded.definitions.begin(), loaded.definitions.end(),
				[](const ModuleDefinition& definition) { return definition.moduleId == sas::ContentId{ kCounterId }; });
			report.Check(found != loaded.definitions.end() && found->sizeVariants.size() == 1 && found->sizeVariants[0].runCounterRules.size() == 1 &&
				found->sizeVariants[0].runCounterRules[0].unitsPerStack == 5000.f && found->sizeVariants[0].runCounterRules[0].damagePerStack == 0.f,
				"the accepted rule keeps its parsed values");

			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the runCounterRules parser", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_run_counter_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare run counter content registration failed" };
		std::string title = "Rare Run Counter E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-run-counter-fixtures";
		Json shipped;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shipped;
		}
		const std::filesystem::path sideFeatures = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/module_side_features.json";
		const auto fixturePath = WriteJson(root, "rare-run-counter-modules.json", FixtureSet(shipped, StandardCounterRules()));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "run counter fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;
		std::string catalogFailure;
		report.Check(content::ModuleContentCatalog::LoadFromFiles(fixturePath, sideFeatures, &catalogFailure),
			"the player's content catalog reads the same fixture modules", catalogFailure);

		ExactChecks(report, application);
		LevelChecks(report, application, 2, 0.012f);
		LevelChecks(report, application, 3, 0.014f);
		SpeedCapChecks(report, application);
		PlayerChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareRunCounterE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
