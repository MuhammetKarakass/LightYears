#include "CampaignStageTrace.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "framework/AssetManager.h"
#include "framework/scene/SceneEntityComponent.h"
#include "level/LevelOne.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "framework/BackGroundActor.h"
#include "framework/BackgroundLayer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace
{
	using namespace campaign_test;
	Json checks = Json::array();
	void Check(bool passed, const std::string& name)
	{
		checks.push_back({{"name",name},{"passed",passed}});
		if (!passed) throw std::runtime_error(name);
	}
	Json Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path };
		if (!input) throw std::runtime_error("Cannot read " + path.string());
		return Json::parse(input);
	}
	Json Configuration(const CampaignPlan& plan)
	{
		World scratch{ vega::EngineContext::Current() };
		const auto actor = scratch.SpawnActor<CampaignSetupActor>().lock();
		actor->plan = plan;
		actor->AddComponent<SceneEntityComponent>("configuration", "Configuration");
		auto properties = SceneSerializer::Save(scratch)["entities"][0]["properties"];
		for (auto it = properties.begin(); it != properties.end();)
			if (it.key() != "Stages" && it.key() != "Vanguard" && it.key() != "TwinBlade" && it.key() != "Hexagon" && it.key() != "Chaos") it = properties.erase(it); else ++it;
		return properties;
	}
	bool Equivalent(const Json& left, const Json& right)
	{
		if (left.is_number() && right.is_number()) return std::abs(left.get<double>() - right.get<double>()) < 0.000001;
		if (left.type() != right.type()) return false;
		if (left.is_array() || left.is_object())
		{
			if (left.size() != right.size()) return false;
			for (auto it = left.begin(); it != left.end(); ++it)
				if (!Equivalent(it.value(), left.is_object() ? right.at(it.key()) : right.at(it - left.begin()))) return false;
			return true;
		}
		return left == right;
	}
	class HeadlessCampaign : public LevelOne
	{
	public:
		using LevelOne::LevelOne;
		bool completed = false;
	protected:
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
		void AllGameStagesFinished() override { completed = true; }
	};
	class HeadlessPlayerCampaign : public LevelOne
	{
	public:
		using LevelOne::LevelOne;
		bool gameOver = false;
	protected:
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void GameOver() override { gameOver = true; }
	};
	Json RunLevel(const Json& scene)
	{
		campaignTestRandom.seed(1741);
		HeadlessViewport viewport;
		auto level = std::make_shared<HeadlessCampaign>(nullptr, scene);
		level->SetRenderViewport(&viewport);
		level->BeginPlayInternal();
		Json spawns = Json::array();
		std::set<unsigned int> seen;
		int step = 0;
		for (step = 1; step <= 10000; ++step)
		{
			level->TickInternal(0.1f);
			level->GetTimerManager().UpdateTimer(0.1f);
			for (const auto& handle : level->GetActorsByTypeIncludingPending<EnemyActor>())
			{
				const auto enemy = handle.lock();
				if (seen.insert(enemy->GetUniqueID()).second) spawns.push_back({{"step",step},{"id",enemy->GetEnemyId()},
					{"position",{enemy->GetActorLocation().x,enemy->GetActorLocation().y}}});
				// These stages complete by spawn count/time, never by live enemies.
				// Retire observed enemies before combat ticks; this scenario measures
				// the actual level's stage execution, not an unbounded unattended battle.
				enemy->Destroy();
			}
			if (level->completed) break;
		}
		Check(level->completed, "real LevelOne rules finish headlessly");
		return {{"finishStep",step},{"spawns",spawns}};
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	try
	{
		vega::EngineContext context;
		vega::EngineContext::Scope scope{ context };
		vega::InstallVegaEngineAssets();
		const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR };
		const bool postcheck = argc == 4 && std::string{argv[1]} == "--editor-postcheck";
		const auto root = postcheck ? std::filesystem::path{argv[2]} : source / "LightYearsGame/assets";
		const auto artifacts = source / "build/e2e-artifacts/campaign-scene";
		std::filesystem::create_directories(artifacts);
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error("Content bootstrap failed");
		LevelLayout::RegisterTypesAndContent();
		const auto fixture = Read(source / "LightYearsGame/tests/fixtures/level-one-3e26ef91.json");
		const auto legacy = Read(source / "LightYearsGame/tests/fixtures/level-one-3e26ef91-headless.json");
		const auto scene = Read(postcheck ? std::filesystem::path{argv[3]} : root / "scenes/level-one.scene.json");
		const auto layout = LevelLayout::Parse(scene, context, "LevelOne");
		const auto configuration = Configuration(layout.campaign);
		for (const auto& entry : configuration.items()) Check(Equivalent(fixture.at(entry.key()), entry.value()), "pre-change configuration: " + entry.key());
		Check(configuration == Configuration(CampaignPlan{}), "scene configuration exactly equals C++ fallback defaults");
		const auto makeStages = [&](World& world)
		{
			std::vector<shared_ptr<GameStage>> stages;
			for (const auto& entry : layout.campaign.stages) stages.push_back(layout.campaign.CreateStage(world, entry));
			return stages;
		};
		const auto observed = Run(makeStages);
		Check(observed == legacy, "scene stage sequence/timing and every enemy spawn equal frozen pre-change headless trace");
		Check(Run(LegacyStages) == legacy, "legacy constructor path still equals frozen pre-change headless trace");
		std::ofstream{ artifacts / (postcheck ? "editor-observed.json" : "scene-observed.json") } << observed.dump(2) << '\n';
		const auto integrated = RunLevel(scene);
		Check(integrated["spawns"] == legacy["spawns"] && integrated["finishStep"] == legacy["events"].back()["step"], "actual LevelOne factory/order/tick integration has identical spawns and completion timing");
		std::ofstream{ artifacts / (postcheck ? "editor-level.json" : "scene-level.json") } << integrated.dump(2) << '\n';
		World roundTrip{ context };
		Check(layout.LoadEntities(roundTrip).warnings.empty(), "CampaignSetupActor loads without warnings");
		const auto saved = SceneSerializer::Save(roundTrip, scene["settings"]);
		Check(Configuration(LevelLayout::Parse(saved, context, "LevelOne").campaign) == configuration, "all reflected stage/parameter lists round trip");
		World reopened{ context };
		Check(SceneSerializer::Load(reopened, saved).warnings.empty() && SceneSerializer::Save(reopened, scene["settings"]) == saved, "reflected round trip is stable");
		if (postcheck)
		{
			std::ofstream{ artifacts / "editor-checks.json" } << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
			std::cout << "Campaign editor postcheck: " << checks.size() << " checks passed\n";
			return 0;
		}
		auto alternateScene = scene;
		auto& alternate = alternateScene["entities"][0]["properties"];
		alternate["Stages"] = {{{"kind","Wait"},{"duration",0.2f},{"configuration",-1}},
			{{"kind","Hexagon"},{"duration",0},{"configuration",1}}, {{"kind","TwinBlade"},{"duration",0},{"configuration",1}},
			{{"kind","Vanguard"},{"duration",0},{"configuration",1}}};
		alternate["Hexagon"].push_back({{"spawnInterval",0.2f},{"spawnGroups",2}});
		alternate["TwinBlade"].push_back({{"minSpawnInterval",0.2f},{"maxSpawnInterval",0.2f},{"spawnCount",2}});
		alternate["Vanguard"].push_back({{"minSpawnInterval",0.2f},{"maxSpawnInterval",0.2f},{"spawnDistanceToEdge",80},
			{"switchInterval",0.2f},{"rows",1},{"perRow",2}});
		const auto alternatePlan = LevelLayout::Parse(alternateScene, context, "LevelOne").campaign;
		const auto alternateTrace = Run([&](World& world)
		{
			std::vector<shared_ptr<GameStage>> stages;
			for (const auto& entry : alternatePlan.stages) stages.push_back(alternatePlan.CreateStage(world, entry));
			return stages;
		});
		Check(alternateTrace["events"][0]["kind"] == "Wait" && alternateTrace["events"][2]["kind"] == "Hexagon" &&
			alternateTrace["events"][4]["kind"] == "TwinBlade" && alternateTrace["events"][6]["kind"] == "Vanguard" &&
			alternateTrace["events"][1]["step"] == 2 && alternateTrace["spawns"].size() == 10, "authored reordered stages/duration/nonzero configuration indices drive real stage behavior");
		std::ofstream{ artifacts / "alternate-scene.json" } << alternateScene.dump(2) << '\n';
		std::ofstream{ artifacts / "alternate-observed.json" } << alternateTrace.dump(2) << '\n';
		PlayerManager::GetPlayerManager().Reset();
		PlayerManager::GetPlayerManager().SetSelectedShipId("edit-sentinel");
		auto edit = std::make_shared<LevelOne>(nullptr, scene, LevelOne::SettingsApplier{}, true);
		edit->BeginPlayInternal();
		const auto before = SceneSerializer::Save(*edit, scene["settings"]);
		for (int step = 0; step < 100; ++step) { edit->TickInternal(0.1f); edit->GetTimerManager().UpdateTimer(0.1f); }
		Check(edit->IsEditMode() && edit->IsPaused() && !PlayerManager::GetPlayerManager().GetPlayer(), "Edit starts no player or rules");
		Check(edit->GetActorsByTypeIncludingPending<Actor>().size() == 1 && edit->GetActorsByTypeIncludingPending<CampaignSetupActor>().size() == 1, "Edit loads setup entity without stage enemies or decor");
		Check(edit->GetTimerManager().GetTraversalTimerCount() == 0 && SceneSerializer::Save(*edit, scene["settings"]) == before, "Edit starts no stage timers and preserves authored entities");
		Check(PlayerManager::GetPlayerManager().GetSelectedShipId() == "edit-sentinel", "Edit preserves player selection");
		Check(!edit->GetActorsByTypeIncludingPending<CampaignSetupActor>()[0].lock()->FindComponent<SceneEntityComponent>()->HasBegunPlay(), "Edit suppresses component BeginPlay");
		edit.reset();
		PlayerManager::GetPlayerManager().Reset();
		auto reject = [&](const std::string& name, const std::function<void(Json&)>& mutate, const std::string& diagnostic)
		{
			auto invalid = scene;
			mutate(invalid);
			std::string message;
			try { auto refused = std::make_shared<LevelOne>(nullptr, invalid); }
			catch (const std::exception& error) { message = error.what(); }
			Check(!message.empty() && message.find(diagnostic) != std::string::npos, name + ": " + message);
			checks.back()["diagnostic"] = message;
			Check(!PlayerManager::GetPlayerManager().GetPlayer(), name + " starts no player");
		};
		reject("format", [](Json& j) { j["format"] = "other"; }, "format");
		reject("version", [](Json& j) { j["version"] = 2; }, "version");
		reject("fractional version", [](Json& j) { j["version"] = 1.5; }, "version");
		reject("missing scene document", [](Json& j) { j = nullptr; }, "format");
		reject("missing settings", [](Json& j) { j.erase("settings"); }, "settings");
		reject("missing rules", [](Json& j) { j["settings"].erase("levelRules"); }, "levelRules");
		reject("wrong rules owner", [](Json& j) { j["settings"]["levelRules"] = "ArenaTest"; }, "LevelOne");
		reject("nonstring rules", [](Json& j) { j["settings"]["levelRules"] = 1; }, "levelRules");
		reject("missing entities", [](Json& j) { j.erase("entities"); }, "entities");
		reject("missing setup", [](Json& j) { j["entities"] = Json::array(); }, "one CampaignSetupActor");
		reject("duplicate setup", [](Json& j) { auto actor = j["entities"][0]; actor["guid"] = "other"; j["entities"].push_back(actor); }, "one CampaignSetupActor");
		reject("duplicate guid", [](Json& j) { j["entities"].push_back(j["entities"][0]); }, "duplicate");
		reject("empty guid", [](Json& j) { j["entities"][0]["guid"] = ""; }, "guid");
		reject("unknown type", [](Json& j) { j["entities"][0]["type"] = "Unknown"; }, "unknown/non-spawnable");
		reject("unknown kind", [](Json& j) { j["entities"][0]["properties"]["Stages"][1]["kind"] = "Unknown"; }, "Stages[1].kind");
		reject("numeric kind", [](Json& j) { j["entities"][0]["properties"]["Stages"][1]["kind"] = 1; }, "enum label");
		reject("empty stages", [](Json& j) { j["entities"][0]["properties"]["Stages"] = Json::array(); }, "Stages must be nonempty");
		reject("negative duration", [](Json& j) { j["entities"][0]["properties"]["Stages"][0]["duration"] = -1; }, "Stages[0].duration");
		reject("nonfinite duration", [](Json& j) { j["entities"][0]["properties"]["Stages"][0]["duration"] = std::numeric_limits<double>::infinity(); }, "duration");
		reject("string duration", [](Json& j) { j["entities"][0]["properties"]["Stages"][0]["duration"] = "5"; }, "finite float");
		reject("nonwait duration", [](Json& j) { j["entities"][0]["properties"]["Stages"][1]["duration"] = 1; }, "must be zero");
		reject("wait configuration", [](Json& j) { j["entities"][0]["properties"]["Stages"][0]["configuration"] = 0; }, "must be -1");
		for (int stage : {1,3,5,7})
		{
			reject("out of range configuration " + std::to_string(stage), [stage](Json& j) { j["entities"][0]["properties"]["Stages"][stage]["configuration"] = 1; }, "outside the list");
			reject("negative configuration " + std::to_string(stage), [stage](Json& j) { j["entities"][0]["properties"]["Stages"][stage]["configuration"] = -1; }, "outside the list");
		}
		reject("fractional configuration", [](Json& j) { j["entities"][0]["properties"]["Stages"][1]["configuration"] = 0.5; }, "int32");
		reject("multiple chaos", [](Json& j) { j["entities"][0]["properties"]["Stages"].push_back(j["entities"][0]["properties"]["Stages"][7]); }, "at most one Chaos");
		for (const auto* list : {"Stages","Vanguard","TwinBlade","Hexagon","Chaos"})
		{
			reject(std::string{"missing list "} + list, [list](Json& j) { j["entities"][0]["properties"].erase(list); }, std::string{list} + " is required");
			reject(std::string{"list kind "} + list, [list](Json& j) { j["entities"][0]["properties"][list] = 1; }, "must be a list");
			for (const auto& field : configuration.at(list).at(0).items())
				reject(std::string{"missing field "} + list + "." + field.key(), [list, key = field.key()](Json& j) { j["entities"][0]["properties"][list][0].erase(key); }, field.key() + " is required");
		}
		for (const auto* list : {"Vanguard","TwinBlade","Hexagon","Chaos"})
		{
			reject(std::string{"empty referenced list "} + list, [list](Json& j) { j["entities"][0]["properties"][list] = Json::array(); }, "outside the list");
			for (const auto& field : configuration.at(list).at(0).items())
				reject(std::string{"negative parameter "} + list + "." + field.key(), [list, key = field.key()](Json& j) { j["entities"][0]["properties"][list][0][key] = -1; }, list);
		}
		for (const auto* list : {"Vanguard","TwinBlade"})
			reject(std::string{"reversed interval "} + list, [list](Json& j) { j["entities"][0]["properties"][list][0]["maxSpawnInterval"] = 0.1; }, "ordered");
		for (const auto* list : {"Vanguard","TwinBlade","Hexagon","Chaos"})
		{
			const std::string key = std::string{list} == "Vanguard" || std::string{list} == "TwinBlade" ? "minSpawnInterval" : "spawnInterval";
			reject(std::string{"zero interval "} + list, [list,key](Json& j) { j["entities"][0]["properties"][list][0][key] = 0; }, "positive");
		}
		reject("overflow rows", [](Json& j) { j["entities"][0]["properties"]["Vanguard"][0]["rows"] = std::numeric_limits<int>::max(); }, "overflow");
		reject("overflow hexagon", [](Json& j) { j["entities"][0]["properties"]["Hexagon"][0]["spawnGroups"] = std::numeric_limits<int>::max(); }, "overflow");
		reject("chaos interval exhausted", [](Json& j) { j["entities"][0]["properties"]["Chaos"][0]["spawnIntervalDecrement"] = 0.3; }, "difficulty 11");
		reject("unknown property", [](Json& j) { j["entities"][0]["properties"]["typo"] = 1; }, "typo");
		reject("unknown struct field", [](Json& j) { j["entities"][0]["properties"]["Stages"][0]["typo"] = 1; }, "typo");
		reject("oversize list", [](Json& j) { j["entities"][0]["properties"]["Stages"] = std::vector<Json>(4097,j["entities"][0]["properties"]["Stages"][0]); }, "4096");
		reject("incompatible player marker", [](Json& j) { j["entities"].push_back({{"guid","start"},{"type","PlayerStart"}}); }, "incompatible");
		reject("incompatible arena setup", [](Json& j) { j["entities"][0]["type"] = "ArenaSetupActor"; }, "unknown or read-only");
		reject("incompatible player actor", [](Json& j) { j["entities"].push_back({{"guid","player"},{"type","PlayerSpaceShip"}}); }, "incompatible");
		reject("missing prefab", [](Json& j) { j["entities"][0].erase("type"); j["entities"][0]["prefab"] = "prefabs/missing.prefab.json"; }, "cannot resolve");
		auto repairScene = scene;
		repairScene["entities"][0]["properties"]["Stages"][0]["duration"] = -1;
		auto repair = std::make_shared<LevelOne>(nullptr, repairScene, LevelOne::SettingsApplier{}, true);
		repair->BeginPlayInternal();
		Check(repair->IsEditMode() && repair->GetLoadResult().warnings.empty(), "invalid gameplay values remain editable for repair");
		repair.reset();

		HeadlessViewport viewport;
		auto playerLevel = std::make_shared<HeadlessPlayerCampaign>(nullptr, scene);
		playerLevel->SetRenderViewport(&viewport);
		playerLevel->BeginPlayInternal();
		Check(playerLevel->mPlayerSpaceShip.lock() && PlayerManager::GetPlayerManager().GetPlayer()->GetLifeCount() == 2, "campaign starts original player flow with two remaining lives");
		Check(playerLevel->GetActorsByTypeIncludingPending<BackGroundActor>().size() == 1 && playerLevel->GetActorsByTypeIncludingPending<BackgroundLayer>().size() == 2, "Play retains the original background and two BackgroundLayer actors");
		for (int lives = 1; lives >= 0; --lives)
		{
			const auto previous = playerLevel->mPlayerSpaceShip.lock();
			previous->Destroy();
			const auto next = playerLevel->mPlayerSpaceShip.lock();
			Check(next && next->GetUniqueID() != previous->GetUniqueID() && PlayerManager::GetPlayerManager().GetPlayer()->GetLifeCount() == static_cast<unsigned int>(lives), "death immediately respawns original campaign player");
		}
		playerLevel->mPlayerSpaceShip.lock()->Destroy();
		Check(playerLevel->mPlayerSpaceShip.expired() && !playerLevel->gameOver, "final death schedules game over instead of respawning");
		context.GlobalTimers().UpdateTimer(2.99f);
		Check(!playerLevel->gameOver, "game over does not fire before original three seconds");
		context.GlobalTimers().UpdateTimer(0.02f);
		Check(playerLevel->gameOver, "game over fires after original three seconds");
		playerLevel.reset();
		PlayerManager::GetPlayerManager().Reset();
		context.GlobalTimers().ClearAllTimers();
		const auto fallbackRoot = artifacts / "absent-scene-assets";
		std::filesystem::create_directories(fallbackRoot / "scenes");
		context.Assets().SetAssetRootDirectory(fallbackRoot.generic_string() + "/");
		std::filesystem::remove(fallbackRoot / "scenes/level-one.scene.json");
		const auto fallback = LevelLayout::ReadLevelOne(context);
		Check(!fallback.fromScene && Configuration(fallback.campaign) == configuration, "absent scene alone selects identical code defaults");
		std::ofstream{ fallbackRoot / "scenes/level-one.scene.json" } << "{malformed";
		std::string error;
		try { LevelLayout::ReadLevelOne(context); } catch (const std::exception& failure) { error = failure.what(); }
		Check(error.find("level-one.scene.json") != std::string::npos, "existing malformed scene rejects with its path instead of falling back");
		std::ofstream{ fallbackRoot / "scenes/level-one.scene.json" } << repairScene.dump();
		error.clear();
		try { LevelLayout::ReadLevelOne(context); } catch (const std::exception& failure) { error = failure.what(); }
		Check(error.find("Stages[0].duration") != std::string::npos, "existing invalid scene never falls back");
		std::filesystem::remove(fallbackRoot / "scenes/level-one.scene.json");
		std::filesystem::create_directory(fallbackRoot / "scenes/level-one.scene.json");
		error.clear();
		try { LevelLayout::ReadLevelOne(context); } catch (const std::exception& failure) { error = failure.what(); }
		Check(error.find("level-one.scene.json") != std::string::npos, "existing unreadable scene rejects with its path");
		std::filesystem::remove(fallbackRoot / "scenes/level-one.scene.json");
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		std::ofstream{ artifacts / "checks.json" } << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
		std::cout << "Campaign scene: " << checks.size() << " checks passed\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		// Native Windows filesystem diagnostics can contain non-UTF-8 bytes.
		std::cerr << error.what() << '\n' << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
		return 1;
	}
}
