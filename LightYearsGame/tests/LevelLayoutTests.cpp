#include "level/ArenaTestLevel.h"
#include "enemy/DummyEnemy.h"
#include "enemy/EnemyActor.h"
#include "player/PlayerManager.h"
#include "gameFramework/LightYearsAssets.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/camera/CameraComponent.h"
#include "player/PlayerSpaceShip.h"
#include "framework/AssetManager.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <limits>

namespace
{
	using namespace vega;
	using Json = nlohmann::json;
	Json checks = Json::array();
	void Check(bool passed, const std::string& name)
	{
		checks.push_back({ {"name", name}, {"passed", passed} });
		if (!passed) throw std::runtime_error(name);
	}
	Json Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path };
		if (!input) throw std::runtime_error("Cannot read " + path.string());
		return Json::parse(input);
	}
	class HeadlessArena : public ArenaTestLevel
	{
	public:
		using ArenaTestLevel::ArenaTestLevel;
		const ArenaDefinition& Arena() const { return GetArenaDefinition(); }
		PlayerRespawnDefinition Respawn() const { return CreatePlayerRespawnDefinition(); }
		void TickRules(float seconds) { ArenaTestLevel::Tick(seconds); }
	protected:
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
	};

	Json Vec(sf::Vector2f value) { return Json::array({ value.x, value.y }); }
	Json Color(sf::Color value) { return Json::array({ value.r, value.g, value.b, value.a }); }
	Json Configuration(const LevelLayout& layout)
	{
		const auto& bounds = layout.arena.legalBounds;
		const auto& visual = layout.arena.boundaryVisual;
		const auto& respawn = layout.respawn;
		const auto& progression = layout.progression;
		Json waves = Json::array(), anchors = Json::array();
		for (const auto& wave : layout.waves)
		{
			Json entries = Json::array();
			for (const auto& entry : wave.entries) entries.push_back({ {"enemyId", entry.enemyId.ToString()}, {"count", entry.count}, {"spawnInterval", entry.spawnInterval} });
			waves.push_back({ {"entries", entries}, {"interWaveDelay", wave.nextWaveDelay} });
		}
		for (auto point : layout.spawnPoints) anchors.push_back(Vec(point));
		return {
			{"bounds", {bounds.position.x,bounds.position.y,bounds.size.x,bounds.size.y}},
			{"boundary", {{"enabled",visual.enabled},{"visualType",visual.visualType == ArenaBoundaryVisualType::DebugRectangle ? "DebugRectangle" : "Other"},
				{"outOfBoundsMargin",layout.arena.outOfBoundsMargin},{"outOfBoundsTime",layout.arena.outOfBoundsTime},{"outlineColor",Color(visual.outlineColor)},{"warningColor",Color(visual.warningColor)},{"outlineThickness",visual.outlineThickness}}},
			{"respawn", {{"spawnLocation",Vec(respawn.spawnLocation)},{"respawnDelay",respawn.respawnDelay},{"playerIndex",respawn.playerIndex},
				{"createPlayerIfMissing",respawn.createPlayerIfMissing},{"useScreenClamp",respawn.useScreenClamp},{"respawnWhenDestroyed",respawn.respawnWhenDestroyed},
				{"gameOverWhenRespawnFails",respawn.gameOverWhenRespawnFails},{"playerShipId",respawn.playerShipId}}},
			{"music", {{"path",layout.music.path},{"type","Music"},{"volume",layout.music.volume},{"pitch",layout.music.pitch},{"loop",layout.music.loop},{"fadeDuration",layout.music.fadeDuration}}},
			{"enabled",layout.encounterEnabled},{"sequenceMode",layout.sequenceMode == EncounterSequenceMode::EndlessCycle ? "EndlessCycle" : "Finite"},
			{"progression", {{"additionalEnemyEveryWaves",progression.additionalEnemyEveryWaves},{"enemyLevelEveryWaves",progression.enemyLevelEveryWaves},
				{"maximumAdditionalEnemies",progression.maximumAdditionalEnemies},{"maximumEnemyLevel",progression.maximumEnemyLevel},{"seed",progression.seed}}},
			{"maximumConcurrentEnemies",progression.maximumConcurrentEnemies},{"spawnPoints",anchors},{"spawnOffsetSpacing",layout.spawnOffsetSpacing},{"waves",waves},
			{"dummyArmor",layout.dummyArmor}
		};
	}
	bool Equivalent(const Json& expected, const Json& actual)
	{
		// The authored fixture contains decimal source literals, while runtime stores floats.
		if (expected.is_number() && actual.is_number()) return std::abs(expected.get<double>() - actual.get<double>()) < 0.000001;
		if (expected.type() != actual.type()) return false;
		if (expected.is_array() || expected.is_object())
		{
			if (expected.size() != actual.size()) return false;
			for (auto it = expected.begin(); it != expected.end(); ++it)
				if (!Equivalent(it.value(), expected.is_object() ? actual.at(it.key()) : actual.at(it - expected.begin()))) return false;
			return true;
		}
		return expected == actual;
	}
	void CheckConfiguration(const Json& fixture, const LevelLayout& layout)
	{
		const auto configuration = Configuration(layout);
		for (const auto& entry : configuration.items()) Check(Equivalent(fixture.at(entry.key()), entry.value()), "legacy configuration: " + entry.key());
	}
	Json Attributes(const sas::AttributeSystem& attributes)
	{
		Json result = Json::object();
		for (const auto& attribute : attributes.BuildSnapshot()) result[std::string{attribute.id.GetName()}] = {attribute.baseValue,attribute.currentValue,attribute.minValue,attribute.maxValue};
		return result;
	}
	Json DummyState(DummyEnemy& dummy)
	{
		return { {"owner",Attributes(dummy.GetAbilitySystemComponent().GetAttributes())},{"ship",Attributes(dummy.GetShipRuntime().GetAttributes())},
			{"health",dummy.GetHealthComponent().GetHealth()},{"maxHealth",dummy.GetHealthComponent().GetMaxHealth()},{"shield",dummy.GetShieldComponent().GetShield()},
			{"energy",dummy.GetEnergyComponent().GetEnergy()},{"speed",Vec(dummy.GetMovementComponent().GetLegacySpeed())},
			{"grants",dummy.GetAbilitySystemComponent().BuildAbilitySnapshots().size()} };
	}
	void CheckDummies(HeadlessArena& arena, const Json& fixture)
	{
		World original{ nullptr };
		for (const auto& handle : arena.GetActorsByTypeIncludingPending<DummyEnemy>())
		{
			const auto dummy = handle.lock();
			auto definition = *content::ShipContentCatalog::FindById(fixture["dummyShip"].get<std::string>());
			const bool heavy = dummy->GetHealthComponent().GetMaxHealth() == 9000.f;
			definition.health = fixture[heavy ? "heavyDummyHealth" : "smallDummyHealth"].get<float>();
			definition.speed = {};
			definition.baseOwnerAttributes.push_back({OwnerAttributeIds::Armor,fixture["dummyArmor"].get<float>()});
			definition.primaryWeaponId = fixture[heavy ? "heavyDummyWeapon" : "smallDummyWeapon"].get<std::string>();
			const auto direct = original.SpawnActor<DummyEnemy>(definition).lock();
			direct->SetActorLocation(dummy->GetActorLocation());
			Check(DummyState(*direct) == DummyState(*dummy), "full dummy constructor state matches old definition");
			direct->BeginPlayInternal();
			dummy->BeginPlayInternal();
			Check(DummyState(*direct) == DummyState(*dummy), "full dummy BeginPlay state matches old definition");
			direct->TickInternal(0.01f);
			dummy->TickInternal(0.01f);
			Check(DummyState(*direct) == DummyState(*dummy), "full dummy tick state matches old definition");
		}
	}
	Json Observe(HeadlessArena& level)
	{
		Json result;
		const auto& arena = level.Arena();
		result["bounds"] = { arena.legalBounds.position.x, arena.legalBounds.position.y, arena.legalBounds.size.x, arena.legalBounds.size.y };
		result["playerStart"] = Vec(level.Respawn().spawnLocation);
		result["idle"] = level.GetEncounterState() == ArenaEncounterState::Idle;
		result["dummies"] = Json::array();
		for (const auto& handle : level.GetActorsByTypeIncludingPending<DummyEnemy>())
		{
			const auto dummy = handle.lock();
			const auto& attributes = dummy->GetAbilitySystemComponent().GetAttributes();
			result["dummies"].push_back({ {"position", Vec(dummy->GetActorLocation())}, {"health", dummy->GetHealthComponent().GetHealth()},
				{"maxHealth", dummy->GetHealthComponent().GetMaxHealth()}, {"attributeMaxHealth", attributes.GetCurrentValue(OwnerAttributeIds::MaxHealth)},
				{"armor", attributes.GetCurrentValue(OwnerAttributeIds::Armor)}, {"speed", Vec(dummy->GetMovementComponent().GetLegacySpeed())},
				{"velocity", Vec(dummy->GetVelocity())}, {"rotation", dummy->GetActorRotation()},
				{"primary", dummy->GetAbilitySystemComponent().FindAbility<GameAbility>(sas::AbilitySlot::PrimaryFire) != nullptr} });
		}
		std::sort(result["dummies"].begin(), result["dummies"].end(), [](const Json& left, const Json& right) { return left["position"] < right["position"]; });
		if (!level.StartEncounter()) throw std::runtime_error("Headless encounter did not start");
		result["waves"] = Json::array();
		for (int wave = 0; wave < 8; ++wave)
		{
			const auto plan = level.GetEncounterWaveSnapshot();
			Json spawned = Json::array();
			std::set<uint64_t> seen;
			for (int step = 0; step < 500; ++step)
			{
				level.TickRules(0.1f);
				for (const auto& handle : level.GetActorsByTypeIncludingPending<EnemyActor>())
				{
					const auto enemy = handle.lock();
					if (enemy->GetIsPendingDestroy() || !seen.insert(enemy->GetUniqueID()).second) continue;
					spawned.push_back({ {"id", enemy->GetEnemyId()}, {"position", Vec(enemy->GetActorLocation())},
						{"level", enemy->GetEnemyLevel()}, {"seed", enemy->GetSpawnContext().variationSeed}, {"cull", enemy->GetSpawnContext().windowCullEnabled} });
				}
				if (level.GetEncounterWaveSnapshot().state == EncounterWaveState::WaitingForClear) break;
			}
			result["waves"].push_back({ {"number", plan.currentWaveNumber}, {"planned", plan.plannedEnemyCount}, {"level", plan.enemyLevel}, {"spawns", spawned} });
			for (const auto& handle : level.GetActorsByTypeIncludingPending<EnemyActor>()) if (const auto actor = handle.lock(); !actor->GetIsPendingDestroy()) actor->Destroy();
			level.TickRules(1.5f);
		}
		return result;
	}
}

int main(int argc, char** argv)
{
	try
	{
		vega::EngineContext context;
		vega::EngineContext::Scope scope{ context };
		vega::InstallVegaEngineAssets();
		const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR };
		const bool postcheck = argc == 4 && std::string{argv[1]} == "--editor-postcheck";
		const auto root = postcheck ? std::filesystem::path{argv[2]} : source / "LightYearsGame/assets";
		const auto artifacts = source / "build/e2e-artifacts/level-layout";
		std::filesystem::create_directories(artifacts);
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error("Content bootstrap failed");
		LevelLayout::RegisterTypesAndContent();
		const auto fixture = Read(source / "LightYearsGame/tests/fixtures/arena-test-50764d21.json");
		const auto legacy = Read(source / "LightYearsGame/tests/fixtures/arena-test-50764d21-headless.json");
		const auto scene = Read(postcheck ? std::filesystem::path{argv[3]} : root / "scenes/arena-test.scene.json");
		const auto layout = LevelLayout::Parse(scene, context);
		CheckConfiguration(fixture, layout);
		CheckConfiguration(fixture, LevelLayout::Defaults());
		Check(Configuration(layout) == Configuration(LevelLayout::Defaults()), "scene-loaded runtime configuration is exactly identical to C++ legacy defaults");
		vega::PlayerManager::GetPlayerManager().Reset();
		auto arena = std::make_shared<HeadlessArena>(nullptr, scene);
		arena->BeginPlayInternal();
		const auto observed = Observe(*arena);
		Check(observed == legacy, "scene headless output identical to captured pre-change path (24 dummies, eight waves/anchors/seeds)");
		CheckDummies(*arena, fixture);
		std::ofstream{ artifacts / (postcheck ? "editor-observed.json" : "scene-observed.json") } << observed.dump(2) << '\n';
		arena.reset();
		vega::PlayerManager::GetPlayerManager().Reset();
		World roundTrip{ context };
		Check(layout.LoadEntities(roundTrip).warnings.empty(), "reflected setup scene loads without warnings");
		const auto saved = SceneSerializer::Save(roundTrip, scene["settings"]);
		Check(Configuration(LevelLayout::Parse(saved, context)) == Configuration(layout), "both setup actors and all struct/list fields round trip");
		World reopened{ context };
		Check(SceneSerializer::Load(reopened, saved).warnings.empty() && SceneSerializer::Save(reopened, scene["settings"]) == saved, "reflected round trip is stable after sparse input expansion");
		for (const auto& entity : saved["entities"])
		{
			if (!entity.contains("prefab")) continue;
			if (entity["prefab"] == "prefabs/dummies/dummy_standard.prefab.json")
				Check(entity["components"].size() == 1 && entity["components"][0]["properties"].size() == 1,
					"small dummy stays a sparse prefab instance with only Max Health component override");
			else if (entity["prefab"] == "prefabs/dummies/dummy_heavy.prefab.json")
				Check(!entity.contains("components") && entity["properties"].size() == 1 && entity["properties"].contains("Location"),
					"heavy dummy stays a sparse prefab instance with only Location placement");
			else Check(false, "unexpected prefab instance " + entity["prefab"].get<std::string>());
		}
		if (postcheck)
		{
			std::ofstream{ artifacts / "editor-checks.json" } << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
			std::cout << "Editor scene postcheck: " << checks.size() << " checks passed\n";
			return 0;
		}
		auto editScene = scene;
		editScene["entities"][1]["properties"]["Enabled"] = true;
		auto edit = std::make_shared<ArenaTestLevel>(nullptr, editScene, ArenaTestLevel::SettingsApplier{}, true);
		PlayerManager::GetPlayerManager().SetSelectedShipId("edit-mode-sentinel");
		edit->BeginPlayInternal();
		Check(edit->GetSceneLoadResult().spawned == 27 && edit->GetSceneLoadResult().warnings.empty(), "Edit loads exactly the 27 authored entities");
		const auto before = SceneSerializer::Save(*edit, scene["settings"]);
		edit->TickInternal(1.f);
		edit->SetPaused(false);
		edit->TickInternal(1.f);
		Check(edit->IsPaused() && edit->GetEncounterState() == ArenaEncounterState::Idle && !edit->StartEncounter(), "Edit cannot start encounter even when Enabled or explicitly requested");
		Check(edit->GetActorsByTypeIncludingPending<PlayerSpaceShip>().empty() && edit->GetActorsByTypeIncludingPending<EnemyActor>().empty() && edit->GetActorsByTypeIncludingPending<DummyEnemy>().size() == 24,
			"Edit places all 24 authored dummies and starts neither arena player nor enemy rules");
		Check(PlayerManager::GetPlayerManager().GetSelectedShipId() == "edit-mode-sentinel" && SceneSerializer::Save(*edit, scene["settings"]) == before, "Edit ticks preserve player selection and authored data");
		for (const auto& actor : edit->GetActorsByTypeIncludingPending<Actor>()) Check(!actor.lock()->FindComponent<SceneEntityComponent>()->HasBegunPlay(), "Edit never invokes component BeginPlay");
		edit.reset();
		PlayerManager::GetPlayerManager().Reset();

		auto reject = [&](const std::string& name, const std::function<void(Json&)>& mutate, const std::string& diagnostic)
		{
			auto invalid = scene;
			mutate(invalid);
			std::string message;
			try { LevelLayout::Parse(invalid, context); }
			catch (const std::exception& error) { message = error.what(); }
			Check(!message.empty() && message.find(diagnostic) != std::string::npos, name + " diagnostic: " + message);
			checks.back()["diagnostic"] = message;
			Check(PlayerManager::GetPlayerManager().GetPlayer() == nullptr, name + " validation starts no player");
		};
		reject("format", [](Json& j) { j["format"] = "other"; }, "format");
		reject("format kind", [](Json& j) { j["format"] = 7; }, "format");
		reject("version", [](Json& j) { j["version"] = 2; }, "version");
		reject("fractional version", [](Json& j) { j["version"] = 1.5; }, "version");
		reject("missing settings", [](Json& j) { j.erase("settings"); }, "settings");
		reject("missing bounds", [](Json& j) { j["settings"].erase("bounds"); }, "bounds");
		reject("negative bounds", [](Json& j) { j["settings"]["bounds"][2] = -1; }, "bounds");
		reject("infinite bounds", [](Json& j) { j["settings"]["bounds"][0] = std::numeric_limits<double>::infinity(); }, "bounds");
		reject("level rule selector", [](Json& j) { j["settings"]["levelRules"] = "Other"; }, "levelRules");
		reject("level rule kind", [](Json& j) { j["settings"]["levelRules"] = 7; }, "levelRules");
		reject("route reference", [](Json& j) { j["settings"]["routeStop"] = 999999; }, "routeStop");
		reject("player ship reference", [](Json& j) { j["settings"]["playerShip"] = "Ship.Player.Missing"; }, "playerShip");
		reject("missing entities", [](Json& j) { j.erase("entities"); }, "entities");
		reject("duplicate GUID", [](Json& j) { j["entities"][1]["guid"] = j["entities"][0]["guid"]; }, "duplicate");
		reject("empty GUID", [](Json& j) { j["entities"][1]["guid"] = ""; }, "guid");
		reject("duplicate arena setup", [](Json& j) { auto e = j["entities"][0]; e["guid"] = "extra-arena"; j["entities"].push_back(e); }, "one ArenaSetupActor");
		reject("duplicate encounter setup", [](Json& j) { auto e = j["entities"][1]; e["guid"] = "extra-encounter"; j["entities"].push_back(e); }, "one EncounterSetupActor");
		reject("missing setup", [](Json& j) { j["entities"].erase(0); }, "one ArenaSetupActor");
		reject("missing encounter", [](Json& j) { j["entities"].erase(1); }, "one EncounterSetupActor");
		reject("missing configuration field", [](Json& j) { j["entities"][1]["properties"].erase("MaximumConcurrentEnemies"); }, "MaximumConcurrentEnemies is required");
		reject("missing struct field", [](Json& j) { j["entities"][0]["properties"]["Boundary"].erase("outOfBoundsTime"); }, "outOfBoundsTime is required");
		reject("missing entry count", [](Json& j) { j["entities"][1]["properties"]["Entries"][0].erase("count"); }, "count is required");
		reject("missing start", [](Json& j) { j["entities"].erase(2); }, "one PlayerStart");
		reject("duplicate start", [](Json& j) { auto e = j["entities"][2]; e["guid"] = "extra-start"; j["entities"].push_back(e); }, "one PlayerStart");
		reject("start ship reference", [](Json& j) { j["entities"][2]["properties"]["Ship"] = "Missing"; }, "PlayerStart.Ship");
		reject("nonfinite start", [](Json& j) { j["entities"][2]["properties"]["Location"] = {0,std::numeric_limits<double>::quiet_NaN()}; }, "Location");
		reject("unsupported mode", [](Json& j) { j["entities"][1]["properties"]["SequenceMode"] = "Finite"; }, "SequenceMode");
		reject("empty waves", [](Json& j) { j["entities"][1]["properties"]["Waves"] = Json::array(); }, "Entries.wave");
		reject("empty wave entries", [](Json& j) { j["entities"][1]["properties"]["Entries"] = Json::array(); }, "needs entries");
		reject("bad wave index", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["wave"] = 3; }, "Entries.wave");
		reject("enemy reference", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["enemyId"] = "Enemy.Missing"; }, "enemy_definitions.json");
		reject("nonpositive count", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["count"] = 0; }, "count");
		reject("fractional count", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["count"] = 1.5; }, "int32");
		reject("negative interval", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["spawnInterval"] = -1; }, "spawnInterval");
		reject("overflowing wave count", [](Json& j) { j["entities"][1]["properties"]["Entries"][1]["count"] = std::numeric_limits<int>::max(); }, "overflows");
		reject("count outside int32", [](Json& j) { j["entities"][1]["properties"]["Entries"][0]["count"] = 2147483648ll; }, "int32");
		reject("negative delay", [](Json& j) { j["entities"][1]["properties"]["Waves"][0]["interWaveDelay"] = -1; }, "interWaveDelay");
		for (const auto* key : {"additionalEnemyEveryWaves","enemyLevelEveryWaves","maximumEnemyLevel"})
			reject(std::string{"invalid progression "}+key, [key](Json& j) { j["entities"][1]["properties"]["Progression"][key] = 0; }, "Progression");
		reject("negative additional cap", [](Json& j) { j["entities"][1]["properties"]["Progression"]["maximumAdditionalEnemies"] = -1; }, "Progression");
		reject("negative seed", [](Json& j) { j["entities"][1]["properties"]["Progression"]["seed"] = -1; }, "seed");
		reject("negative concurrency", [](Json& j) { j["entities"][1]["properties"]["MaximumConcurrentEnemies"] = -1; }, "MaximumConcurrentEnemies");
		reject("empty anchors", [](Json& j) { j["entities"][1]["properties"]["SpawnPoints"] = Json::array(); }, "SpawnPoints");
		reject("nonfinite anchor", [](Json& j) { j["entities"][1]["properties"]["SpawnPoints"][0][0] = std::numeric_limits<double>::infinity(); }, "SpawnPoints");
		reject("negative spacing", [](Json& j) { j["entities"][1]["properties"]["SpawnOffsetSpacing"] = -1; }, "SpawnOffsetSpacing");
		reject("enabled kind", [](Json& j) { j["entities"][1]["properties"]["Enabled"] = 1; }, "boolean");
		reject("oversized list", [](Json& j) { j["entities"][1]["properties"]["SpawnPoints"] = std::vector<Json>(4097,Json::array({0,0})); }, "4096");
		reject("vector dimensions", [](Json& j) { j["entities"][1]["properties"]["SpawnPoints"][0] = {0}; }, "dimensions");
		reject("boundary time", [](Json& j) { j["entities"][0]["properties"]["Boundary"]["outOfBoundsTime"] = 0; }, "Boundary");
		reject("boundary thickness", [](Json& j) { j["entities"][0]["properties"]["Boundary"]["outlineThickness"] = -1; }, "outlineThickness");
		reject("negative boundary margin", [](Json& j) { j["entities"][0]["properties"]["Boundary"]["outOfBoundsMargin"] = -1; }, "Boundary");
		reject("color byte", [](Json& j) { j["entities"][0]["properties"]["Boundary"]["outlineColor"][0] = 256; }, "bytes");
		reject("respawn delay", [](Json& j) { j["entities"][0]["properties"]["Respawn"]["respawnDelay"] = -1; }, "Respawn");
		reject("respawn index", [](Json& j) { j["entities"][0]["properties"]["Respawn"]["playerIndex"] = -1; }, "Respawn");
		reject("respawn ship", [](Json& j) { j["entities"][0]["properties"]["Respawn"]["playerShipId"] = "Missing"; }, "playerShipId");
		reject("music pitch", [](Json& j) { j["entities"][0]["properties"]["Music"]["pitch"] = 0; }, "Music");
		reject("music volume", [](Json& j) { j["entities"][0]["properties"]["Music"]["volume"] = 101; }, "Music");
		reject("music fade", [](Json& j) { j["entities"][0]["properties"]["Music"]["fadeDuration"] = -1; }, "Music");
		reject("music path", [](Json& j) { j["entities"][0]["properties"]["Music"]["path"] = ""; }, "Music");
		reject("small dummy armor", [](Json& j) { j["entities"][0]["properties"]["DummyArmor"] = -1; }, "DummyArmor");
		reject("unknown struct field", [](Json& j) { j["entities"][0]["properties"]["Boundary"]["typo"] = 1; }, "typo");
		reject("unknown actor property", [](Json& j) { j["entities"][0]["properties"]["typo"] = 1; }, "typo");
		reject("struct kind", [](Json& j) { j["entities"][0]["properties"]["Boundary"] = 1; }, "object");
		reject("missing prefab", [](Json& j) { j["entities"][3]["prefab"] = "prefabs/missing.prefab.json"; }, "prefab");
		reject("escaping prefab", [](Json& j) { j["entities"][3]["prefab"] = "../outside.prefab.json"; }, "prefab");
		reject("incompatible selector", [](Json& j) { j["entities"][3]["properties"]["Ship"] = "Ship.Player.Fighter.Basic"; }, "incompatible dummy prefab");
		reject("incompatible prefab type", [](Json& j) { j["entities"][3]["prefab"] = "prefabs/ships/player/fighter.prefab.json"; }, "incompatible prefab/actor");
		reject("nonpositive dummy health", [](Json& j) { j["entities"][3]["components"][0]["properties"]["Max Health"] = 0; }, "Max Health");
		reject("required component removal", [](Json& j) { j["entities"][3]["removed"] = {"HealthComponent"}; }, "HealthComponent");
		reject("unknown component", [](Json& j) { j["entities"][3]["components"][0]["type"] = "Unknown"; }, "unknown component");
		reject("component object kind", [](Json& j) { j["entities"][3]["components"] = 1; }, "components");
		reject("ambiguous prefab/type", [](Json& j) { j["entities"][3]["type"] = "DummyEnemy"; }, "exactly one");
		reject("unknown actor", [](Json& j) { j["entities"][0]["type"] = "Unknown"; }, "unknown/non-spawnable");
		// A malformed gameplay field remains editable; only Play performs strict validation.
		auto repairScene = scene;
		repairScene["entities"][1]["properties"]["Entries"][0]["count"] = 0;
		auto repair = std::make_shared<ArenaTestLevel>(nullptr, repairScene, ArenaTestLevel::SettingsApplier{}, true);
		repair->BeginPlayInternal();
		Check(repair->IsEditMode() && repair->GetEncounterState() == ArenaEncounterState::Idle, "invalid gameplay configuration can be opened to repair in Edit");
		repair.reset();
		// The absent-only fallback is exercised with a private asset copy, never source mutation.
		const auto fallbackRoot = artifacts / "absent-scene-assets";
		std::filesystem::create_directories(fallbackRoot);
		std::filesystem::copy(root, fallbackRoot, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		std::filesystem::remove(fallbackRoot / "scenes/arena-test.scene.json");
		context.Assets().SetAssetRootDirectory(fallbackRoot.generic_string() + "/");
		auto fallback = std::make_shared<HeadlessArena>(nullptr);
		Check(!fallback->GetLayout().fromScene, "only an absent scene selects legacy defaults");
		fallback->BeginPlayInternal();
		Check(Observe(*fallback) == legacy, "absent-scene fallback matches captured original headless path");
		fallback.reset();
		PlayerManager::GetPlayerManager().Reset();
		std::ofstream{ fallbackRoot / "scenes/arena-test.scene.json" } << "{malformed";
		try { auto invalid = std::make_shared<HeadlessArena>(nullptr); Check(false, "malformed scene must not fall back"); }
		catch (const std::exception& error) { Check(std::string{error.what()}.find("arena-test.scene.json") != std::string::npos, "malformed file diagnostic includes scene path"); }
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		vega::PlayerManager::GetPlayerManager().Reset();
		std::ofstream{ artifacts / "checks.json" } << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
		std::cout << "Level layout: " << checks.size() << " checks passed\n";
		return 0;
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
