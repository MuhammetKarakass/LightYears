#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameplay/MovementComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace rare_movement_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kOmniId = "Module.Movement.RareOmniFixture.Rare";
		constexpr const char* kTrimId = "Module.Movement.RareTrimFixture.Rare";
		constexpr const char* kVectorSyncId = "Ability.Movement.VectorSync.Basic";
		constexpr float kTolerance = 0.5f;

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		ly::List<ly::ModuleDefinition> gFixtureDefinitions;
		// Fighter movement block from ships.json, read by main before any check runs.
		float gShipForward = 0.f;
		float gShipReverse = 0.f;
		float gShipStrafe = 0.f;

		struct Thrust
		{
			float forward = 0.f;
			float reverse = 0.f;
			float strafe = 0.f;
		};

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

		Json MovementModule(const char* id, const char* displayName, Json movementRules)
		{
			return Json{
				{ "id", id }, { "displayName", displayName }, { "rarity", "Rare" }, { "levelCount", 3 },
				{ "family", "Ability.Movement" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ Json{
					{ "shape", Json::array({ Json::array({ 0, 0 }) }) },
					{ "movementRules", std::move(movementRules) }
				} }) }
			};
		}

		// Shipped modules plus two test-only Movement modules.
		Json FixtureSet(const Json& shipped, const Json& omniRules, const Json& trimRules)
		{
			Json modules = shipped;
			modules["modules"].push_back(MovementModule(kOmniId, "Rare movement omni fixture", omniRules));
			modules["modules"].push_back(MovementModule(kTrimId, "Rare movement trim fixture", trimRules));
			return modules;
		}

		Json ThrustJson(const Thrust& thrust)
		{
			return Json{ { "forward", thrust.forward }, { "reverse", thrust.reverse }, { "strafe", thrust.strafe } };
		}

		bool Near(float left, float right, float tolerance = kTolerance)
		{
			return std::abs(left - right) <= tolerance;
		}

		bool SameThrust(const Thrust& left, const Thrust& right, float tolerance = kTolerance)
		{
			return Near(left.forward, right.forward, tolerance) && Near(left.reverse, right.reverse, tolerance) && Near(left.strafe, right.strafe, tolerance);
		}

		// Thrust the movement component resolves right now (ship bonus and module bonus applied).
		Thrust Resolved(const Fixture& f)
		{
			const MovementComponent& movement = f.ship->GetMovementComponent();
			return Thrust{ movement.ResolveForwardThrust(), movement.ResolveReverseThrust(), movement.ResolveStrafeThrust() };
		}

		// Raw movement attributes before any thrust multiplier or equalization.
		Thrust Attributes(const Fixture& f)
		{
			const ShipMovementAttributes& attributes = f.ship->GetMovementComponent().GetAttributes();
			return Thrust{ attributes.forwardThrust.currentValue, attributes.reverseThrust.currentValue, attributes.strafeThrust.currentValue };
		}

		// Expected thrust: the ship's own thrust bonus plus moduleBonus, applied to the raw attributes, and with
		// reverse and strafe taking the forward value when equalize is set.
		Thrust Expected(const Fixture& f, const Thrust& base, float moduleBonus, bool equalize)
		{
			const float multiplier = std::max(0.f, 1.f + f.ship->GetRuntimeModifiers().GetThrustBonus() + moduleBonus);
			const Thrust scaled{ base.forward * multiplier, base.reverse * multiplier, base.strafe * multiplier };
			return equalize ? Thrust{ scaled.forward, scaled.forward, scaled.forward } : scaled;
		}

		// Removes every placement the loadout holds (the shipped default modules bring movement ratings).
		void ClearLoadout(Fixture& f)
		{
			ly::List<std::uint32_t> installed;
			for (const ModulePlacement& placement : f.loadout.GetPlacements()) installed.push_back(placement.instanceId);
			for (const std::uint32_t id : installed) f.Remove(id);
		}

		void ParsedChecks(Report& report, const ly::List<ly::ModuleDefinition>& definitions)
		{
			const auto found = std::find_if(definitions.begin(), definitions.end(),
				[](const ModuleDefinition& definition) { return definition.moduleId == sas::ContentId{ kOmniId }; });
			report.Check(found != definitions.end() && !found->sizeVariants.empty(), "Omni fixture parses into a movement rule");
			const auto& rules = found->sizeVariants[0].movementRules;
			report.Check(rules.size() == 1 && rules[0].equalizeDirections &&
				rules[0].thrustMultiplierByLevel.values == ly::List<float>{ 0.70f, 0.715f, 0.73f },
				"level table and equalizeDirections parse exactly", Json{ { "count", rules.size() } });
		}

		void ThrustChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MovementComponent& movement = f.ship->GetMovementComponent();
			report.Check(!movement.HasModuleDirectionalThrustSync() && movement.GetModuleThrustBonus() == 0.f,
				"shipped default loadout carries no module movement rule");

			ClearLoadout(f);
			const Thrust attributes = Attributes(f);
			const Thrust bare = Resolved(f);
			const auto& abilityAttributes = f.ship->GetAbilitySystemComponent().GetAttributes();
			const float horizontalRating = abilityAttributes.HasAttribute(OwnerAttributeIds::MoveSpeedHorizontal)
				? abilityAttributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal) : 0.f;
			const float verticalRating = abilityAttributes.HasAttribute(OwnerAttributeIds::MoveSpeedVertical)
				? abilityAttributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical) : 0.f;
			report.Check(horizontalRating == 0.f && verticalRating == 0.f, "bare Fighter has no movement rating contribution",
				Json{ { "horizontal", horizontalRating }, { "vertical", verticalRating } });
			const Thrust shipsJson{ gShipForward, gShipReverse, gShipStrafe };
			report.Check(SameThrust(attributes, shipsJson, 0.01f), "unruled Fighter thrust equals ships.json",
				Json{ { "attributes", ThrustJson(attributes) }, { "shipsJson", ThrustJson(shipsJson) } });
			report.Check(SameThrust(bare, Expected(f, attributes, 0.f, false), 1e-3f), "unruled Fighter resolves to its raw thrust",
				Json{ { "resolved", ThrustJson(bare) } });

			// Installs Omni at the given level and checks its bonus and the equalized thrust.
			const auto omniAt = [&](int levelValue, float multiplier) -> std::uint32_t
			{
				const std::uint32_t instance = f.Install(kOmniId, 0, levelValue);
				const Thrust resolved = Resolved(f);
				const std::string label = "Omni level " + std::to_string(levelValue);
				report.Check(Near(movement.GetModuleThrustBonus(), multiplier - 1.f, 1e-4f) && movement.HasModuleDirectionalThrustSync(),
					label + " sets its thrust bonus and equalization", Json{ { "bonus", movement.GetModuleThrustBonus() } });
				const Thrust expected = Expected(f, attributes, multiplier - 1.f, true);
				report.Check(SameThrust(resolved, expected), label + " makes forward, reverse and strafe equal",
					Json{ { "resolved", ThrustJson(resolved) }, { "expected", ThrustJson(expected) } });
				return instance;
			};
			// Removing the module must restore the bare values exactly.
			const auto restored = [&](const std::string& name)
			{
				const Thrust resolved = Resolved(f);
				report.Check(SameThrust(resolved, bare, 1e-3f) && movement.GetModuleThrustBonus() == 0.f && !movement.HasModuleDirectionalThrustSync(),
					name, Json{ { "resolved", ThrustJson(resolved) } });
			};

			f.Remove(omniAt(1, 0.70f));
			restored("removing Omni level one restores the bare thrust");
			f.Remove(omniAt(2, 0.715f));
			restored("removing Omni level two restores the bare thrust");
			f.Remove(omniAt(3, 0.73f));
			restored("removing Omni level three restores the bare thrust");

			// VectorSync and module equalization are separate sources.
			const std::uint32_t omni = omniAt(1, 0.70f);
			report.Check(!movement.HasDirectionalThrustSync(), "module equalization does not register as a VectorSync source");
			movement.SetDirectionalThrustSync(kVectorSyncId, true);
			report.Check(movement.HasDirectionalThrustSync() && movement.HasModuleDirectionalThrustSync(),
				"VectorSync on keeps module equalization as a separate source");
			movement.SetDirectionalThrustSync(kVectorSyncId, false);
			const Thrust moduleOnly = Expected(f, attributes, -0.30f, true);
			report.Check(!movement.HasDirectionalThrustSync() && movement.HasModuleDirectionalThrustSync() && SameThrust(Resolved(f), moduleOnly),
				"VectorSync off leaves the module equalization in place", Json{ { "resolved", ThrustJson(Resolved(f)) } });
			f.Remove(omni);

			movement.SetDirectionalThrustSync(kVectorSyncId, true);
			const Thrust abilityOnly = Expected(f, attributes, 0.f, true);
			report.Check(SameThrust(Resolved(f), abilityOnly) && !movement.HasModuleDirectionalThrustSync(),
				"VectorSync alone equalizes directions as before", Json{ { "resolved", ThrustJson(Resolved(f)) } });
			f.Remove(omniAt(1, 0.70f));
			report.Check(movement.HasDirectionalThrustSync() && SameThrust(Resolved(f), abilityOnly),
				"removing the module keeps the VectorSync equalization");
			movement.SetDirectionalThrustSync(kVectorSyncId, false);
			restored("VectorSync off with no module restores the bare thrust");

			// Ten install and removal cycles must leave the handle count and thrust unchanged.
			const std::size_t handlesBefore = f.ship->GetModuleApplier().GetOwnedHandleCount();
			ly::List<std::size_t> handlesPerCycle;
			bool cyclesStable = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				f.Remove(omniAt(1, 0.70f));
				const std::size_t handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
				handlesPerCycle.push_back(handles);
				cyclesStable = cyclesStable && handles == handlesBefore && SameThrust(Resolved(f), bare, 1e-3f) &&
					movement.GetModuleThrustBonus() == 0.f && !movement.HasModuleDirectionalThrustSync();
			}
			report.Check(cyclesStable && f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesBefore,
				"ten install and removal cycles keep handle counts and thrust stable",
				Json{ { "handlesBefore", handlesBefore }, { "handlesPerCycle", handlesPerCycle } });

			// Several movement rules: the smallest multiplier wins, nothing stacks, and any equalize flag holds.
			const std::uint32_t omniPair = omniAt(1, 0.70f);
			const std::uint32_t trim = f.Install(kTrimId, 0, 1);
			report.Check(Near(movement.GetModuleThrustBonus(), -0.30f, 1e-4f) && movement.HasModuleDirectionalThrustSync(),
				"smallest thrustMultiplier wins across modules and rules do not stack", Json{ { "bonus", movement.GetModuleThrustBonus() } });
			f.Remove(omniPair);
			const Thrust trimOnly = Expected(f, attributes, -0.15f, false);
			report.Check(Near(movement.GetModuleThrustBonus(), -0.15f, 1e-4f) && !movement.HasModuleDirectionalThrustSync() &&
				SameThrust(Resolved(f), trimOnly),
				"removing Omni leaves Trim alone with its own multiplier and no equalization",
				Json{ { "resolved", ThrustJson(Resolved(f)) }, { "expected", ThrustJson(trimOnly) } });
			f.Remove(trim);
			restored("removing every movement rule restores the bare thrust");
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped)
		{
			const Json validTrim = Json::array({ Json{ { "thrustMultiplier", 0.85 } } });
			const auto reject = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", FixtureSet(shipped, rules, validTrim));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "movementRules loader rejects " } + name, loaded.error);
			};
			reject("zero-multiplier", Json::array({ Json{ { "thrustMultiplier", 0.0 } } }));
			reject("above-one", Json::array({ Json{ { "thrustMultiplier", 1.2 }, { "equalizeDirections", true } } }));
			reject("missing-multiplier", Json::array({ Json{ { "equalizeDirections", true } } }));
			reject("empty-entry", Json::array({ Json::object() }));
			reject("unknown-field", Json::array({ Json{ { "thrustMultiplier", 0.7 }, { "thrust", 1 } } }));
			reject("no-effect-entry", Json::array({ Json{ { "thrustMultiplier", 1.0 } } }));
			reject("equalize-not-boolean", Json::array({ Json{ { "thrustMultiplier", 0.7 }, { "equalizeDirections", "yes" } } }));
			reject("string-multiplier", Json::array({ Json{ { "thrustMultiplier", "0.7" } } }));
			reject("short-level-table", Json::array({ Json{ { "thrustMultiplier", Json::array({ 0.7, 0.6 }) } } }));
			reject("level-above-one", Json::array({ Json{ { "thrustMultiplier", Json::array({ 0.7, 1.5, 0.73 }) } } }));
			reject("level-zero", Json::array({ Json{ { "thrustMultiplier", Json::array({ 0.7, 0.0, 0.73 }) } } }));
			reject("rules-not-array", Json::object());
			reject("entry-not-object", Json::array({ 1.0 }));

			const auto accepted = [&](const char* name, const Json& rules)
			{
				const auto path = WriteJson(root, std::string{ "accept-" } + name + ".json", FixtureSet(shipped, rules, validTrim));
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(loaded.Succeeded(), std::string{ "movementRules loader accepts " } + name, loaded.error);
			};
			accepted("scalar-with-equalize", Json::array({ Json{ { "thrustMultiplier", 0.7 }, { "equalizeDirections", true } } }));
			accepted("scalar-only", Json::array({ Json{ { "thrustMultiplier", 0.9 } } }));

			const auto shippedLoaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(shippedLoaded.Succeeded(), "shipped modules still load through the movementRules parser", shippedLoaded.error);
		}
	}
}

int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	using namespace rare_movement_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare movement content registration failed" };
		std::string title = "Rare Movement Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path dataDir = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-movement-fixtures";
		Json shipped;
		{
			std::ifstream input{ dataDir / "modules.json" };
			input >> shipped;
		}
		Json ships;
		{
			std::ifstream input{ dataDir / "ships.json" };
			input >> ships;
		}
		for (const Json& ship : ships["ships"])
		{
			if (ship.value("id", std::string{}) != "Ship.Player.Fighter.Basic") continue;
			gShipForward = ship["movement"]["forwardThrust"].get<float>();
			gShipReverse = ship["movement"]["reverseThrust"].get<float>();
			gShipStrafe = ship["movement"]["strafeThrust"].get<float>();
		}
		report.Check(gShipForward > 0.f && gShipReverse > 0.f && gShipStrafe > 0.f, "ships.json provides the Fighter movement block",
			Json{ { "forward", gShipForward }, { "reverse", gShipReverse }, { "strafe", gShipStrafe } });

		const Json omniRules = Json::array({ Json{ { "thrustMultiplier", Json::array({ 0.70, 0.715, 0.73 }) }, { "equalizeDirections", true } } });
		const Json trimRules = Json::array({ Json{ { "thrustMultiplier", 0.85 } } });
		const auto fixturePath = WriteJson(root, "rare-movement-modules.json", FixtureSet(shipped, omniRules, trimRules));
		const auto loaded = content::ModuleLoader::LoadFromFile(fixturePath);
		report.Check(loaded.Succeeded(), "rare movement fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;

		ParsedChecks(report, gFixtureDefinitions);
		ThrustChecks(report, application);
		LoaderChecks(report, root, shipped);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareMovementRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
