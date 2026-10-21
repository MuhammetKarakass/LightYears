#include "gameFramework/LightYearsAssets.h"
#include "enemy/EnemyActor.h"
#include "gameConfigs/GameplayTags.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ShipPrefabSpawner.h"
#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "player/PlayerManager.h"
#include "reflection/GameReflection.h"
#include <script/ScriptRuntime.h>
#include <framework/AssetManager.h>
#include <framework/LightComponent.h>
#include <framework/SpriteComponent.h>
#include <framework/World.h>
#include <framework/reflection/Reflection.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <fstream>
#include <iostream>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif

namespace
{
	using namespace vega;
	using Json = nlohmann::json;
	Json checks = Json::array();
	int failures = 0;

	class EditWorld final : public World
	{
	public:
		using World::World;
		bool IsEditMode() const override { return true; }
	};

	void Check(bool passed, const std::string& name, Json actual = nullptr)
	{
		checks.push_back({ { "name", name }, { "passed", passed }, { "actual", std::move(actual) } });
		if (!passed) { ++failures; std::cerr << name << '\n'; }
	}

	Json Read(const std::filesystem::path& path)
	{
		Json value;
		std::ifstream{ path } >> value;
		return value;
	}

	void Write(const std::filesystem::path& path, const Json& value)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path };
		output << value.dump(2, ' ', false, Json::error_handler_t::replace) << '\n'; // Native Windows filesystem diagnostics can contain non-UTF-8 bytes.
		if (!output) throw std::runtime_error("Cannot write fixture: " + path.string());
	}

	Json Attributes(const sas::AttributeSystem& attributes)
	{
		Json result = Json::object();
		for (const auto& value : attributes.BuildSnapshot()) result[std::string{ value.id.GetName() }] = { value.baseValue, value.currentValue, value.minValue, value.maxValue };
		return result;
	}

	Json Snapshot(EnemyActor& actor)
	{
		Json components = Json::object(), abilities = Json::array(), lights = Json::array();
		actor.ForEachComponent([&](ActorComponent& component)
		{
			if (dynamic_cast<SceneEntityComponent*>(&component)) return;
			const auto* type = actor.GetEngineContext().Types().FindMostDerived(component);
			const auto name = type ? type->name : typeid(component).name();
			components[name] = components.value(name, 0) + 1;
		});
		for (const auto& grant : actor.GetAbilitySystemComponent().BuildAbilitySnapshots())
		{
			const auto* ability = actor.GetAbilitySystemComponent().GetAbility(grant.handle);
			abilities.push_back({ { "id", grant.abilityId }, { "level", ability ? ability->GetLevel() : 0 }, { "slot", ability ? static_cast<int>(ability->GetRuntimeSlot()) : -1 } });
		}
		if (auto* lightComponent = actor.FindComponent<LightComponent>())
			for (const auto& tag : lightComponent->GetLightsByBaseTag(GameTags::Ship::Engine_Main))
			{
				const auto& light = *lightComponent->GetLightData(tag);
				lights.push_back({ { "offset", { light.offset.x, light.offset.y } }, { "color", { light.color.r, light.color.g, light.color.b, light.color.a } },
					{ "size", { light.size.x, light.size.y } }, { "intensity", light.intensity }, { "stretch", light.shouldStretch }, { "trail", light.useComplexTrail },
					{ "taper", light.taperAmount }, { "softness", light.edgeSoftness }, { "roundness", light.shapeRoundness }, { "enabled", light.isEnabled } });
			}
		const auto* sprite = actor.FindComponent<SpriteComponent>();
		const auto& movement = actor.GetMovementComponent().GetAttributes();
		const auto* combat = actor.GetEnemyRuntime().GetCurrentProfile();
		const auto* behavior = actor.GetEnemyBehaviorRuntime().GetProfile();
		const auto& context = actor.GetSpawnContext();
		return {
			{ "components", components }, { "owner", Attributes(actor.GetAbilitySystemComponent().GetAttributes()) }, { "ship", Attributes(actor.GetShipRuntime().GetAttributes()) },
			{ "abilities", abilities }, { "lights", lights }, { "enemy", actor.GetEnemyId() }, { "combat", actor.GetCombatProfileId() },
			{ "activeCombat", combat ? combat->profileId : "" }, { "behavior", behavior ? behavior->profileId : "" },
			{ "context", { context.level, context.variationSeed, context.windowCullEnabled } }, { "encounterMultiplier", actor.GetEnemyRuntime().GetEncounterDamageMultiplier() },
			{ "outgoingDamage", actor.GetCombatRuntime().GetOutgoingDamageMultiplier() }, { "xp", actor.GetShipXPReward() }, { "score", actor.GetScoreAmt() }, { "collisionDamage", actor.GetCollisionDamage() },
			{ "health", actor.GetHealthComponent().GetHealth() }, { "shield", actor.GetShieldComponent().GetShield() }, { "energy", actor.GetEnergyComponent().GetEnergy() },
			{ "texture", sprite ? sprite->GetTexturePath() : "" }, { "scale", sprite ? Json{ sprite->GetScale().x, sprite->GetScale().y } : Json{} },
			{ "location", { actor.GetActorLocation().x, actor.GetActorLocation().y } }, { "rotation", actor.GetActorRotation() },
			{ "movement", { movement.forwardThrust.currentValue, movement.reverseThrust.currentValue, movement.strafeThrust.currentValue, movement.maxSpeed.currentValue,
				movement.angularTurnSpeed.currentValue, movement.angularTurnResponsiveness.currentValue, movement.linearDamping.currentValue, movement.inputResponsiveness.currentValue,
				movement.mouseAimDeadZone.currentValue, movement.horizontalRatingToThrust, movement.verticalRatingToThrust, movement.ratingToMaxSpeed, movement.movementRatingDiminishingScale } },
			{ "movementMode", static_cast<int>(actor.GetMovementMode()) }, { "domain", static_cast<int>(actor.GetSimulationTimeDomain()) },
			{ "collisionLayer", actor.GetCollisionLayer() }, { "collisionMask", actor.GetCollisionMask() }, { "physics", actor.IsPhysicsEnabled() }
		};
	}

	std::shared_ptr<EnemyActor> Direct(World& world, const EnemyDefinition& definition, float multiplier = 1.f, const EnemySpawnContext& context = {})
	{
		return world.SpawnActor<EnemyActor>(*content::ShipContentCatalog::FindById(definition.shipId), definition.id,
			*content::EnemyCombatProfileCatalog::FindById(definition.combatProfileId), *content::EnemyContentCatalog::FindBehaviorById(definition.behaviorProfileId), multiplier, context).lock();
	}

	std::size_t LiveActors(World& world)
	{
		std::size_t count = 0;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}

	void Compare(EnemyActor& direct, EnemyActor& prefab, const std::string& label)
	{
		const auto expected = Snapshot(direct), actual = Snapshot(prefab);
		for (const auto& field : expected.items()) Check(field.value() == actual.at(field.key()), label + " " + field.key(), actual.at(field.key()));
	}

	void CheckSparse(const Json& document, const EnemyDefinition& enemy)
	{
		Check(document.at("base") == Json{ { "type", "EnemyActor" } } && document.at("properties") == Json{ { "Enemy ID", enemy.id } }, enemy.id + " sparse base and selector", document);
		for (const auto& field : document.items())
			Check(field.key() == "format" || field.key() == "version" || field.key() == "name" || field.key() == "base" || field.key() == "properties", enemy.id + " no snapshot field " + field.key());
	}

	void Equivalence(EngineContext& engine, const EnemyDefinition& enemy, const std::filesystem::path& root)
	{
		Check(enemy.prefabPath.has_value(), enemy.id + " selects prefab");
		if (!enemy.prefabPath) return;
		std::string reason;
		CheckSparse(Read(root / *enemy.prefabPath), enemy);
		Check(ValidateEnemyPrefab(engine, root, *enemy.prefabPath, enemy.id, &reason), enemy.id + " validator accepts", reason);
		{
			EngineContext isolated;
			EngineContext::Scope isolatedScope{ isolated };
			InstallVegaEngineAssets();
			isolated.Assets().SetAssetRootDirectory(root.generic_string() + "/");
			World world{ isolated };
			Check(!SpawnEnemyPrefab(world, *enemy.prefabPath, enemy.id, &reason).expired(), enemy.id + " standalone spawn registers owning context", reason);
		}
		const auto* property = engine.Types().Find("EnemyActor")->FindProperty("Enemy ID");
		const EnemySpawnContext contexts[]{ {}, { 7, 8675309, false }, { 3, 0, false }, { 2, 17, true } };
		const float multipliers[]{ 1.f, 1.75f, 0.f, 0.5f };
		for (std::size_t i = 0; i < std::size(contexts); ++i)
		{
			const auto label = enemy.id + " context " + std::to_string(i);
			World directWorld{ engine }, prefabWorld{ engine };
			const auto direct = Direct(directWorld, enemy, multipliers[i], contexts[i]);
			const sf::Vector2f location{ 451.f, 673.f };
			direct->SetActorLocation(location);
			const auto prefab = i == 0 ? content::SpawnEnemy(prefabWorld, enemy.id, location, multipliers[i]).lock()
				: content::SpawnEnemy(prefabWorld, enemy.id, location, multipliers[i], contexts[i]).lock();
			Check(prefab != nullptr, label + " runtime spawn");
			if (!prefab) continue;
			Compare(*direct, *prefab, label + " constructed");
			Check(prefab->GetEnemyRuntime().IsSafeEmpty() && !prefab->GetEnemyBehaviorRuntime().GetProfile() && !prefab->IsPhysicsEnabled() &&
				!prefab->GetMovementComponent().HasBegunPlay() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0 && PlayerManager::GetPlayerManager().GetPlayers().empty(), label + " construction starts no gameplay");
			Check(property->construction && property->SetValue(prefab.get(), enemy.id, &reason), label + " construction metadata and idempotence");
			Check(!property->SetValue(prefab.get(), std::string{ "Enemy.Other.Basic" }, &reason) && reason.find("construction-only") != std::string::npos && prefab->GetEnemyId() == enemy.id, label + " refuses changed ID", reason);
			const auto saved = SceneSerializer::Save(prefabWorld);
			Check(saved["entities"].size() == 1 && saved["entities"][0].at("prefab") == *enemy.prefabPath && !saved["entities"][0].contains("components"), label + " sparse scene");
			World reopened{ engine };
			const auto loaded = SceneSerializer::Load(reopened, saved);
			Check(loaded.spawned == 1 && loaded.warnings.empty() && SceneSerializer::Save(reopened) == saved, label + " scene reopens");
			// Scene placement has neutral context; caller state is deliberately never serialized.
			const auto reopenedEnemy = reopened.GetActorsByTypeIncludingPending<EnemyActor>().front().lock();
			Check(reopenedEnemy->GetEnemyLevel() == 1 && reopenedEnemy->GetSpawnContext().variationSeed == 0, label + " caller context stays runtime-only");
			PrefabLibrary library{ engine, root };
			auto sparse = library.DiffType(SceneSerializer::SaveEntity(*prefab, true));
			Check(sparse && sparse.warnings.empty(), label + " sparse type authoring succeeds");
			if (sparse)
			{
				sparse.entity["type"] = "EnemyActor"; // Diff omits the unchanged type; Document needs it, as in the editor.
				CheckSparse(PrefabLibrary::Document(sparse.entity, enemy.id), enemy);
			}
			direct->BeginPlayInternal();
			prefab->BeginPlayInternal();
			Check(direct->GetEnemyRuntime().IsReady() && prefab->GetEnemyRuntime().IsReady() && prefab->GetEnemyBehaviorRuntime().GetProfile() &&
				prefab->GetEnemyBehaviorRuntime().GetProfile()->profileId == enemy.behaviorProfileId, label + " profiles initialize");
			Compare(*direct, *prefab, label + " begun");
			Check(prefab->GetEnemyRuntime().GetEncounterDamageMultiplier() == multipliers[i], label + " caller encounter multiplier");
			bool refused = false;
			try { prefab->ConfigureSpawnContext(1.f, {}); } catch (const std::exception&) { refused = true; }
			Check(refused, label + " refuses context after BeginPlay");
			direct->Tick(0.016f);
			prefab->Tick(0.016f);
			Compare(*direct, *prefab, label + " tick");
			const auto* chassis = content::ShipContentCatalog::FindById(enemy.shipId);
			Check(chassis->primaryWeaponId.empty() && prefab->GetShipXPReward() == chassis->shipXPReward && prefab->GetCollisionDamage() == chassis->collisionDamage && prefab->GetScoreAmt() == chassis->scoreAmt, label + " chassis reward/contact ownership");
			prefab->Destroy();
			prefabWorld.TickInternal(0.f);
			prefabWorld.GetTimerManager().UpdateTimer(0.f);
			Check(LiveActors(prefabWorld) == 0 && prefabWorld.GetActorCountIncludingPending() == 0 && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0, label + " destruction reclaims actors and timers");
		}
	}

	void Validation(EngineContext& engine, const std::filesystem::path& root, const EnemyDefinition& enemy)
	{
		const auto valid = Read(root / *enemy.prefabPath);
		const std::string path = "prefabs/enemies/validation.prefab.json";
		const auto reject = [&](const Json& value, const std::string& name)
		{
			Write(root / path, value);
			std::string reason;
			Check(!ValidateEnemyPrefab(engine, root, path, enemy.id, &reason) && !reason.empty(), "Reject " + name, reason);
			EditWorld world{ engine };
			Check(SpawnEnemyPrefab(world, path, enemy.id, &reason).expired() && world.GetActorCountIncludingPending() == 0, "Rejected " + name + " creates no actors", reason);
		};
		auto candidate = valid; candidate["properties"].erase("Enemy ID"); reject(candidate, "missing selector");
		candidate = valid; candidate["properties"]["Enemy ID"] = "Enemy.Other.Basic"; reject(candidate, "different selector");
		candidate = valid; candidate["properties"]["Enemy ID"] = 3; reject(candidate, "non-string selector");
		candidate = valid; candidate["base"]["type"] = "DummyEnemy"; reject(candidate, "wrong type");
		candidate = valid; candidate["base"]["prefab"] = *enemy.prefabPath; reject(candidate, "ambiguous base");
		for (const auto& name : { "Ship XP Reward", "Collision Damage", "Combat Profile ID", "Enemy Level", "Render Enabled", "Location" })
		{
			candidate = valid; candidate["properties"][name] = 4; reject(candidate, std::string{ "property " } + name);
		}
		for (const auto& name : { "components", "scripts", "children", "removed" })
		{
			candidate = valid; candidate[name] = Json::array(); reject(candidate, std::string{ "composition " } + name);
		}
		candidate = valid; candidate["base"] = { { "prefab", path } }; reject(candidate, "cycle");
		candidate = valid; candidate["base"] = { { "prefab", "../../../outside.prefab.json" } }; reject(candidate, "escaping parent");
		std::string reason;
		for (const auto& badPath : { std::string{ "../outside.prefab.json" }, std::string{ "prefabs/enemies/missing.prefab.json" }, (root / *enemy.prefabPath).generic_string() })
			Check(!ValidateEnemyPrefab(engine, root, badPath, enemy.id, &reason) && !reason.empty(), "Reject invalid asset path " + badPath, reason);
		candidate = valid; candidate["base"] = { { "prefab", *enemy.prefabPath } }; candidate.erase("properties");
		Write(root / path, candidate);
		Check(ValidateEnemyPrefab(engine, root, path, enemy.id, &reason), "Accept inherited selector", reason);
		for (const auto& choice : content::EnemyContentCatalog::GetDefinitions())
		{
			candidate["properties"] = { { "Enemy ID", choice.id } };
			Write(root / path, candidate);
			Check(ValidateEnemyPrefab(engine, root, path, choice.id, &reason), choice.id + " accepts variant selector", reason);
			World directWorld{ engine }, prefabWorld{ engine };
			const auto direct = Direct(directWorld, choice);
			const auto prefab = SpawnEnemyPrefab(prefabWorld, path, choice.id, &reason).lock();
			Check(prefab != nullptr, choice.id + " variant constructs", reason);
			if (prefab) Compare(*direct, *prefab, choice.id + " variant");
		}
	}

	void FailureCleanup(EngineContext& engine, const EnemyDefinition& enemy)
	{
		EditWorld world{ engine };
		const auto sentinel = world.SpawnActor<Actor>().lock();
		sentinel->SetActorLocation({ 17.f, 29.f });
		world.unresolvedPrefabs = Json::array({ { { "prefab", "keep-missing.prefab.json" } } });
		const auto unresolved = world.unresolvedPrefabs;
		const auto original = engine.SceneSerialization().spawnFactories.at("EnemyActor");
		for (int mode = 0; mode != 4; ++mode)
		{
			SceneSerializer::RegisterSpawnFactory("EnemyActor", [&](World& target, const Json& properties) -> std::shared_ptr<Actor>
			{
				auto actor = original(target, properties);
				if (&target != &world) return actor; // Resolve's scratch construction must succeed first.
				if (mode == 0) target.SpawnActor<Actor>();
				if (mode == 1) { target.SpawnActor<Actor>(); throw std::runtime_error("Injected partial construction failure"); }
				if (mode == 2) actor->Destroy();
				if (mode == 3) target.unresolvedPrefabs.push_back({ { "prefab", "injected.prefab.json" } });
				return actor;
			}, engine);
			std::string reason;
			Check(SpawnEnemyPrefab(world, *enemy.prefabPath, enemy.id, &reason).expired() && !reason.empty(), "Injected failure " + std::to_string(mode) + " refuses spawn", reason);
			Check(LiveActors(world) == 1 && !sentinel->GetIsPendingDestroy() && sentinel->GetActorLocation() == sf::Vector2f{ 17.f, 29.f } && world.unresolvedPrefabs == unresolved && world.GetTimerManager().GetTraversalTimerCount() == 0, "Injected failure " + std::to_string(mode) + " preserves caller and destroys all new actors");
			world.TickInternal(0.f);
			Check(world.GetActorCountIncludingPending() == 1, "Injected failure " + std::to_string(mode) + " normal cleanup reclaims storage");
		}
		SceneSerializer::RegisterSpawnFactory("EnemyActor", original, engine);
		const auto before = world.GetActorCountIncludingPending();
		Check(content::SpawnEnemy(world, enemy.id, {}, -1.f).expired() && content::SpawnEnemy(world, enemy.id, {}, std::numeric_limits<float>::quiet_NaN()).expired() &&
			content::SpawnEnemy(world, enemy.id, {}, 1.f, { 0, 0, false }).expired() && content::SpawnEnemy(world, "Enemy.Missing.Basic", {}).expired() && world.GetActorCountIncludingPending() == before, "Invalid caller inputs create no actors");
	}

	void CatalogAndFallback(EngineContext& engine, const std::filesystem::path& root)
	{
		const auto definitionsPath = root / "content/data/enemy_definitions.json", behaviorPath = root / "content/data/enemy_behavior_profiles.json";
		const auto authored = Read(definitionsPath);
		std::string reason;
		for (const Json& invalid : { Json{ "" }, Json{ nullptr }, Json{ 7 }, Json{ "../escape.prefab.json" }, Json{ "prefabs/ships/player/fighter.prefab.json" } })
		{
			auto candidate = authored; candidate["enemies"][0]["prefabPath"] = invalid.at(0);
			Write(root / "invalid-enemies.json", candidate);
			Check(!content::EnemyContentCatalog::LoadFromFiles(root / "invalid-enemies.json", behaviorPath, &reason), "Catalog rejects invalid prefabPath", reason);
		}
		auto legacy = authored;
		for (auto& enemy : legacy["enemies"]) enemy.erase("prefabPath");
		Write(root / "legacy-enemies.json", legacy);
		Check(content::EnemyContentCatalog::LoadFromFiles(root / "legacy-enemies.json", behaviorPath, &reason), "Legacy catalog accepts absent prefabPath", reason);
		const auto definitions = content::EnemyContentCatalog::GetDefinitions();
		for (const auto& enemy : definitions)
		{
			World directWorld{ engine }, fallbackWorld{ engine };
			const EnemySpawnContext context{ 5, 991, false };
			const auto direct = Direct(directWorld, enemy, 1.4f, context);
			const auto fallback = content::SpawnEnemy(fallbackWorld, enemy.id, {}, 1.4f, context).lock();
			Check(fallback && !fallback->FindComponent<SceneEntityComponent>(), enemy.id + " actual direct fallback");
			if (fallback)
			{
				direct->BeginPlayInternal(); fallback->BeginPlayInternal();
				Compare(*direct, *fallback, enemy.id + " fallback");
			}
		}
		Check(content::EnemyContentCatalog::LoadFromFiles(definitionsPath, behaviorPath, &reason), "Restore prefab catalog", reason);
	}

	void Waves(EngineContext& engine)
	{
		World world{ engine };
		EncounterWaveRuntime runtime;
		EnemyWaveDefinition wave;
		for (const auto& enemy : content::EnemyContentCatalog::GetDefinitions()) wave.entries.push_back({ sas::ContentId{ enemy.id }, 1, 0.f });
		std::string reason;
		Check(runtime.Start({ wave }, [&](const sas::ContentId& id, std::size_t index, const EnemySpawnContext& context)
		{
			auto encounterContext = context;
			encounterContext.windowCullEnabled = false; // ArenaTestLevel owns this policy, not EncounterWaveRuntime.
			return content::SpawnEnemy(world, id.ToString(), { static_cast<float>(index) * 100.f, 500.f }, 1.3f, encounterContext);
		}, EncounterProgression{ 2, 2, 0, 7, 1234, 24 }, EncounterSequenceMode::Finite, &reason), "Wave starts", reason);
		runtime.Tick(1.f);
		Check(!runtime.HasFailed() && runtime.GetOwnedEnemies().size() == content::EnemyContentCatalog::GetDefinitions().size(), "Wave spawns all catalog ids", runtime.GetFailureReason());
		for (const auto& weak : runtime.GetOwnedEnemies()) if (const auto actor = weak.lock())
		{
			Check(actor->FindComponent<SceneEntityComponent>() && !actor->GetSpawnContext().windowCullEnabled && actor->GetSpawnContext().variationSeed != 0, actor->GetEnemyId() + " wave context reaches prefab");
			actor->BeginPlayInternal();
			Check(actor->GetEnemyRuntime().IsReady(), actor->GetEnemyId() + " wave enemy begins play");
			actor->Destroy();
		}
		runtime.Tick(0.f);
		Check(runtime.IsCompleted(), "Wave observes destroyed prefab enemies and completes");
	}
}

int RunTests(int argc, char** argv)
{
	using namespace vega;
	EngineContext engine;
	EngineContext::Scope scope{ engine };
	InstallVegaEngineAssets();
	const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	const auto artifacts = std::filesystem::path{ ENEMY_PREFAB_ARTIFACT_DIR };
	std::filesystem::create_directories(artifacts);
	const bool editorPostcheck = argc == 4 && std::string{ argv[1] } == "--accept-sparse-root";
	const auto root = editorPostcheck ? std::filesystem::path{ argv[2] } : artifacts / "assets";
	if (!editorPostcheck) std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	engine.Assets().SetAssetRootDirectory(root.generic_string() + "/");
	RegisterGameSceneTypes(engine);
	if (!GameContentBootstrap::Register()) { std::cerr << "Enemy prefab bootstrap failed\n"; return 1; }
	const auto definitions = content::EnemyContentCatalog::GetDefinitions();
	Check(definitions.size() == 3, "All three shipped enemies present");
	{
		World world{ engine };
		const auto* type = engine.Types().Find("EnemyActor");
		const auto actor = std::dynamic_pointer_cast<EnemyActor>(type->create(world));
		Check(type->spawnable && actor && actor->GetEnemyId() == definitions.front().id && actor->GetEnemyRuntime().IsSafeEmpty(), "Editor class creation constructs neutral default enemy");
	}
	if (editorPostcheck)
	{
		script::ScriptRuntime::Get(engine).SetRoot(root);
		script::RegisterScriptTypes(engine);
		const auto scene = Read(argv[3]);
		World reopened{ engine };
		const auto loaded = SceneSerializer::Load(reopened, scene);
		Check(loaded.warnings.empty(), "Editor scene loads without warnings", loaded.warnings);
		for (const auto& enemy : definitions)
		{
			CheckSparse(Read(root / *enemy.prefabPath), enemy);
			std::string reason;
			Check(ValidateEnemyPrefab(engine, root, *enemy.prefabPath, enemy.id, &reason), enemy.id + " editor prefab validates", reason);
			World directWorld{ engine }, prefabWorld{ engine };
			const auto direct = Direct(directWorld, enemy), prefab = SpawnEnemyPrefab(prefabWorld, *enemy.prefabPath, enemy.id, &reason).lock();
			Check(prefab != nullptr, enemy.id + " editor prefab spawns", reason);
			if (prefab) Compare(*direct, *prefab, enemy.id + " editor");
			int instances = 0;
			for (const auto& entity : scene.at("entities")) if (entity.value("prefab", std::string{}) == *enemy.prefabPath)
			{
				++instances;
				Check(!entity.contains("components") && !entity.contains("scripts") && entity.value("properties", Json::object()).size() <= 1, enemy.id + " editor scene remains sparse", entity);
				const auto properties = entity.value("properties", Json::object());
				for (const auto& property : properties.items()) Check(property.key() == "Location", enemy.id + " editor scene authors only placement");
			}
			Check(instances == 1, enemy.id + " editor scene retains one instance");
		}
		Write(root / "enemy-authoring-check.json", { { "checks", checks }, { "failures", failures } });
	}
	else
	{
		for (const auto& enemy : definitions) Equivalence(engine, enemy, root);
		Validation(engine, root, definitions.front());
		FailureCleanup(engine, definitions.front());
		CatalogAndFallback(engine, root);
		Waves(engine);
		Write(artifacts / "enemy-prefab-results.json", { { "checks", checks }, { "failures", failures } });
	}
	std::cout << checks.size() << " enemy prefab E2E checks; " << failures << " failures\n";
	return failures ? 1 : 0;
}

int main(int argc, char** argv)
{
#ifdef _MSC_VER
	_set_error_mode(_OUT_TO_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
	try { return RunTests(argc, argv); }
	catch (const std::exception& error) { std::cerr << "Enemy prefab E2E exception: " << error.what() << '\n' << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n'; return 1; }
}
