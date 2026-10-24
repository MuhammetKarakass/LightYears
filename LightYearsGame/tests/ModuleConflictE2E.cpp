#include "gameFramework/LightYearsAssets.h"
#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenHUDController.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "presentation/hud/modules/ModuleScreenViewModel.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
	using Json = nlohmann::json;
	using namespace vega;

	// Two probe modules are copies of the shipped piercing tip. Probe X lists Probe Y; Probe Y lists nothing.
	const char* kProbeX = "Module.Primary.ConflictProbeX.Test";
	const char* kProbeY = "Module.Primary.ConflictProbeY.Test";
	const char* kTip = "Module.Primary.PiercingTip.Basic";

	class HeadlessModuleLevel final : public vega::GameLevel
	{
	public:
		explicit HeadlessModuleLevel(vega::Application* application) : GameLevel{ application } {}
		bool Dispatch(const sf::Event& event) { return DispatchEvent(event); }
		std::shared_ptr<vega::ModuleScreenHUDController> controller;

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override
		{
			controller = std::make_shared<vega::ModuleScreenHUDController>(std::weak_ptr<vega::GameHUD>{});
			AddHUDController(controller);
		}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		vega::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{ { "test", "ModuleConflict" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, const Json& actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = actual;
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open ModuleConflict artifact path" };
		output << artifact.dump(2) << '\n';
	}

	std::filesystem::path DataPath()
	{
		return std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
	}

	// The shipped modules.json with two probe modules appended. A null conflict list leaves the key out.
	Json MakeProbe(const Json& base, const char* id, const char* name, const Json& conflicts)
	{
		Json probe = base;
		probe["id"] = id;
		probe["displayName"] = name;
		probe.erase("conflictsWith");
		if (!conflicts.is_null()) probe["conflictsWith"] = conflicts;
		return probe;
	}

	Json MakeRoot(const Json& xConflicts, const Json& yConflicts)
	{
		std::ifstream input{ DataPath() / "modules.json" };
		Json root;
		input >> root;
		Json base;
		for (const Json& module : root.at("modules"))
			if (module.at("id") == kTip) base = module;
		root["modules"].push_back(MakeProbe(base, kProbeX, "Probe X", xConflicts));
		root["modules"].push_back(MakeProbe(base, kProbeY, "Probe Y", yConflicts));
		return root;
	}

	std::filesystem::path WriteFixture(const std::filesystem::path& directory, const std::string& name, const Json& root)
	{
		std::filesystem::create_directories(directory);
		const std::filesystem::path path = directory / name;
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		output << root.dump(2);
		return path;
	}

	const ModuleDefinition* FindDefinition(const content::ModuleLoader::Result& result, const char* id)
	{
		for (const ModuleDefinition& definition : result.definitions)
			if (definition.moduleId == sas::ContentId{ std::string{ id } }) return &definition;
		return nullptr;
	}

	ModuleRulesContext MakeRules()
	{
		return ModuleRulesContext{
			*content::ShipContentCatalog::GetPlayerFighterDefinition().schematic,
			[](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); },
			content::ModuleContentCatalog::GetZoneTable(),
			[](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); }
		};
	}

	// First grid origin (rotation 0) where the item passes every placement rule against the loadout.
	std::optional<sf::Vector2i> FindLegalOrigin(const ModuleRulesContext& rules, const ModuleLoadout& loadout, const ModulePlacement& item)
	{
		ModulePlacement candidate = item;
		candidate.rotationQuarterTurns = 0;
		for (int y = 0; y < rules.schematic.height; ++y)
			for (int x = 0; x < rules.schematic.width; ++x)
			{
				candidate.origin = { x, y };
				if (ModulePlacementRules::CanPlace(rules, loadout, candidate) == ModulePlacementFailure::None) return candidate.origin;
			}
		return std::nullopt;
	}

	std::uint32_t FindInventoryId(const Player& player, const std::string& moduleId)
	{
		for (const ModulePlacement& item : player.GetModuleLoadout().GetInventory())
			if (item.moduleId.ToString() == moduleId) return item.instanceId;
		return 0;
	}

	const ModuleEntryView* FindEntry(const List<ModuleEntryView>& entries, std::uint32_t instanceId)
	{
		for (const ModuleEntryView& entry : entries)
			if (entry.instanceId == instanceId) return &entry;
		return nullptr;
	}

	void LoaderChecks(Report& report, const std::filesystem::path& directory)
	{
		const auto valid = WriteFixture(directory, "valid.json", MakeRoot(Json::array({ std::string{ kProbeY } }), Json::array()));
		const auto loaded = content::ModuleLoader::LoadFromFile(valid);
		report.Check(loaded.Succeeded(), "conflictsWith fixture loads through the real loader", Json(loaded.error));
		const ModuleDefinition* probeX = FindDefinition(loaded, kProbeX);
		const ModuleDefinition* probeY = FindDefinition(loaded, kProbeY);
		report.Check(probeX && probeY && probeX->conflictsWith.size() == 1 && probeX->conflictsWith[0] == sas::ContentId{ std::string{ kProbeY } } && probeY->conflictsWith.empty(),
			"conflictsWith parses onto the declaring module only");

		const auto reject = [&](const char* name, const Json& xConflicts)
		{
			const auto path = WriteFixture(directory, std::string{ "reject-" } + name + ".json", MakeRoot(xConflicts, Json::array()));
			const auto result = content::ModuleLoader::LoadFromFile(path);
			report.Check(!result.Succeeded(), std::string{ "conflictsWith rejects " } + name, Json(result.error));
		};
		reject("unknown-id", Json::array({ "Module.Primary.NoSuchModule.Test" }));
		reject("self", Json::array({ std::string{ kProbeX } }));
		reject("duplicate", Json::array({ std::string{ kProbeY }, std::string{ kProbeY } }));
		reject("not-a-list", std::string{ kProbeY });
		reject("non-string-entry", Json::array({ 7 }));

		const auto shipped = content::ModuleLoader::LoadFromFile(DataPath() / "modules.json");
		report.Check(shipped.Succeeded(), "shipped modules.json loads through the real loader", Json(shipped.error));
		// The only shipped conflicts are the deliberate pairs (Rare shield/generator, Epic Turret/Broadside); each member lists only its partner.
		const std::map<std::string, std::string> expectedPartners{
			{ "Module.Movement.MomentumShield.Rare", "Module.Defense.ContinuousFieldGenerator.Rare" },
			{ "Module.Defense.ContinuousFieldGenerator.Rare", "Module.Movement.MomentumShield.Rare" },
			{ "Module.Primary.TurretMount.Epic", "Module.Primary.BroadsideMount.Epic" },
			{ "Module.Primary.BroadsideMount.Epic", "Module.Primary.TurretMount.Epic" } };
		// Maps each declaring module to its single listed partner; an entry with several partners maps to an empty string.
		std::map<std::string, std::string> declaring;
		for (const ModuleDefinition& definition : shipped.definitions)
		{
			if (definition.conflictsWith.empty()) continue;
			declaring[definition.moduleId.ToString()] = definition.conflictsWith.size() == 1 ? definition.conflictsWith[0].ToString() : std::string{};
		}
		report.Check(declaring == expectedPartners, "shipped conflicts are exactly the MomentumShield/ContinuousFieldGenerator and TurretMount/BroadsideMount pairs, each listing only its partner");
	}

	void RuleChecks(Report& report)
	{
		const ModuleRulesContext rules = MakeRules();
		const ModuleDefinition* probeX = rules.lookup(sas::ContentId{ std::string{ kProbeX } });
		const ModuleDefinition* probeY = rules.lookup(sas::ContentId{ std::string{ kProbeY } });
		const ModuleDefinition* tip = rules.lookup(sas::ContentId{ std::string{ kTip } });
		report.Check(probeX && probeY && tip, "probe modules are in the live catalog");

		// X installed first; Y at the same cells is refused and names X.
		ModuleLoadout loadout;
		const std::uint32_t xId = loadout.AddToInventory(rules, *probeX, 0, probeX->rarity, 1);
		const std::uint32_t yId = loadout.AddToInventory(rules, *probeY, 0, probeY->rarity, 1);
		report.Check(xId != 0 && yId != 0, "probe instances enter the inventory");
		const std::optional<sf::Vector2i> origin = FindLegalOrigin(rules, loadout, *loadout.FindInventoryItem(xId));
		report.Check(origin.has_value(), "a legal spot exists for the probe shape");
		report.Check(loadout.Install(rules, xId, *origin, 0) == ModulePlacementFailure::None, "X installs first");
		const ModulePlacementFailure refused = loadout.Install(rules, yId, *origin, 0);
		report.Check(refused == ModulePlacementFailure::ModuleConflict, "Y is refused while X is installed", Json(ModulePlacementRules::Describe(refused)));
		report.Check(ModulePlacementRules::FindConflictingInstances(rules, loadout.GetPlacements(), *loadout.FindInventoryItem(yId)) == List<std::uint32_t>{ xId },
			"the conflict query names X's instance id");
		report.Check(loadout.FindInventoryItem(yId) != nullptr && loadout.FindPlacement(yId) == nullptr, "refused Y stays in the inventory");

		// Removing X frees the cells for Y.
		report.Check(loadout.Uninstall(xId), "X uninstalls");
		report.Check(loadout.Install(rules, yId, *origin, 0) == ModulePlacementFailure::None, "with X gone, Y installs on the same cells");

		// Reverse order: Y first. X does not list Y, but the relation is symmetric.
		ModuleLoadout reverse;
		const std::uint32_t yFirst = reverse.AddToInventory(rules, *probeY, 0, probeY->rarity, 1);
		const std::uint32_t xSecond = reverse.AddToInventory(rules, *probeX, 0, probeX->rarity, 1);
		report.Check(reverse.Install(rules, yFirst, *origin, 0) == ModulePlacementFailure::None, "Y installs first in the reverse order");
		report.Check(reverse.Install(rules, xSecond, *origin, 0) == ModulePlacementFailure::ModuleConflict, "X is refused while Y is installed");
		report.Check(ModulePlacementRules::FindConflictingInstances(rules, reverse.GetPlacements(), *reverse.FindInventoryItem(xSecond)) == List<std::uint32_t>{ yFirst },
			"the reverse conflict query names Y's instance id");

		// A placed module moving over its own cells is not a conflict with itself.
		report.Check(reverse.Move(rules, yFirst, *origin, 0) == ModulePlacementFailure::None, "moving Y over its own cells is not a conflict");

		// Unrelated modules are unaffected by the probe.
		ModuleLoadout mixed;
		const std::uint32_t tipId = mixed.AddToInventory(rules, *tip, 0, tip->rarity, 1);
		const std::uint32_t xMixed = mixed.AddToInventory(rules, *probeX, 0, probeX->rarity, 1);
		const std::uint32_t yMixed = mixed.AddToInventory(rules, *probeY, 0, probeY->rarity, 1);
		const std::optional<sf::Vector2i> mixedOrigin = FindLegalOrigin(rules, mixed, *mixed.FindInventoryItem(xMixed));
		report.Check(mixedOrigin.has_value() && mixed.Install(rules, xMixed, *mixedOrigin, 0) == ModulePlacementFailure::None, "X installs in the mixed loadout");
		const std::optional<sf::Vector2i> tipOrigin = FindLegalOrigin(rules, mixed, *mixed.FindInventoryItem(tipId));
		report.Check(tipOrigin.has_value() && mixed.Install(rules, tipId, *tipOrigin, 0) == ModulePlacementFailure::None, "an unrelated module installs beside an installed probe");
		report.Check(ModulePlacementRules::FindConflictingInstances(rules, mixed.GetPlacements(), *mixed.FindPlacement(tipId)).empty(), "the unrelated module has no conflicts");
		report.Check(ModulePlacementRules::FindConflictingInstances(rules, mixed.GetPlacements(), *mixed.FindInventoryItem(yMixed)) == List<std::uint32_t>{ xMixed },
			"the conflict query for Y lists only the probe X");
	}

	void PresenterChecks(Report& report, Application& application)
	{
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		Player& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(ship), "conflict presenter test spawns a player ship");
		level->BeginPlayInternal();
		level->TickInternal(0.f);
		const auto presenter = level->controller->GetPresenter();
		const auto model = presenter->GetViewModel();

		const sf::Event tabDown{ sf::Event::KeyPressed{ sf::Keyboard::Key::Tab } };
		const sf::Event tabUp{ sf::Event::KeyReleased{ sf::Keyboard::Key::Tab } };
		level->Dispatch(tabDown);
		level->Dispatch(tabDown);
		level->TickInternal(0.f);
		report.Check(model->isOpen, "module screen opens for the conflict checks");
		level->Dispatch(tabUp);

		const std::uint32_t xId = FindInventoryId(player, kProbeX);
		const std::uint32_t yId = FindInventoryId(player, kProbeY);
		report.Check(xId != 0 && yId != 0, "the probe modules reach the player inventory");
		const std::optional<ModuleRulesContext> rules = player.BuildModuleRules();
		report.Check(rules.has_value(), "the player exposes module rules");
		const std::optional<sf::Vector2i> origin = FindLegalOrigin(*rules, player.GetModuleLoadout(), *player.GetModuleLoadout().FindInventoryItem(xId));
		report.Check(origin.has_value() && player.TryInstallModule(xId, *origin, 0) == ModulePlacementFailure::None, "X installs through the player");
		level->TickInternal(0.f);

		// Hovering Y over X's cells previews the conflict by name and marks X's instance id.
		presenter->PickUpInventory(yId);
		presenter->SetHoverCell(*origin);
		level->TickInternal(0.f);
		report.Check(model->heldInstanceId == yId && model->preview.active && !model->preview.legal && model->preview.message == "Conflicts with Probe X",
			"hovering a conflicting cell previews the conflict by name", Json(model->preview.message));
		report.Check(model->preview.conflictingInstanceIds == List<std::uint32_t>{ xId }, "the preview carries the blocking instance id");
		const ModuleEntryView* yEntry = FindEntry(model->inventory, yId);
		report.Check(yEntry && yEntry->conflictingInstanceIds == List<std::uint32_t>{ xId }, "Y's inventory entry names X");

		// Dropping on the conflict fails, says why, and keeps Y held and in the inventory.
		const std::size_t placedBefore = player.GetModuleLoadout().GetPlacements().size();
		report.Check(!presenter->DropOnGrid(), "dropping Y on the conflicting cells fails");
		level->TickInternal(0.f);
		report.Check(model->statusMessage == "Conflicts with Probe X" && model->heldInstanceId == yId && player.GetModuleLoadout().GetPlacements().size() == placedBefore &&
			player.GetModuleLoadout().FindInventoryItem(yId) != nullptr,
			"a refused drop shows the conflict name, keeps the hold and installs nothing", Json(model->statusMessage));

		// Moving X over its own cells is not a conflict.
		report.Check(presenter->PickUpAtCell(*origin), "clicking the installed probe X picks it up");
		presenter->SetHoverCell(*origin);
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.conflictingInstanceIds.empty(), "X's own cells are not a conflict while moving X");
		report.Check(presenter->DropOnGrid(), "moving X back onto its own cells succeeds");

		// Uninstalling X lets Y install on the same cells.
		report.Check(presenter->PickUpAtCell(*origin) && presenter->DropOnInventory(), "dropping X on the inventory uninstalls it");
		presenter->PickUpInventory(yId);
		presenter->SetHoverCell(*origin);
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.conflictingInstanceIds.empty(), "with X gone, Y previews as legal");
		report.Check(presenter->DropOnGrid(), "with X gone, Y installs through the presenter");
		level->TickInternal(0.f);
		report.Check(player.GetModuleLoadout().FindPlacement(yId) != nullptr, "player loadout holds Y installed");
	}

	void RunPresenterChecks(Report& report)
	{
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, std::string{ "Module Conflict E2E" }, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		PresenterChecks(report, application);
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2)
	{
		std::cerr << "Usage: LightYearsModuleConflictE2ETests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };

		const std::filesystem::path fixtureDirectory = std::filesystem::absolute(artifactPath).parent_path() / "module-conflict-fixtures";
		LoaderChecks(report, fixtureDirectory);

		// Replace the module catalog with the shipped content plus the probes; the side features stay shipped.
		const auto probeModules = WriteFixture(fixtureDirectory, "modules-with-probes.json", MakeRoot(Json::array({ std::string{ kProbeY } }), Json::array()));
		std::string failure;
		report.Check(content::ModuleContentCatalog::LoadFromFiles(probeModules, DataPath() / "module_side_features.json", &failure), "probe catalog loads over the shipped content", Json(failure));

		RuleChecks(report);
		RunPresenterChecks(report);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "ModuleConflict tests failed: " << exception.what() << '\n';
	}
	try
	{
		WriteArtifact(artifactPath, report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write ModuleConflict artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
