#include "gameFramework/LightYearsAssets.h"
#include "enemy/DummyEnemy.h"
#include "gameConfigs/GameplayTags.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ShipPrefabSpawner.h"
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
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif

namespace
{
	using namespace vega;
	using Json = nlohmann::json;
	constexpr const char* DummyShipId = "Ship.Enemy.RangeKeeper.Basic";
	constexpr const char* DummyPrefabPath = "prefabs/dummies/dummy_standard.prefab.json";
	Json checks = Json::array();
	int failures = 0;

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
		output << value.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
		if (!output) throw std::runtime_error("Cannot write fixture: " + path.string());
	}

	Json Attributes(const sas::AttributeSystem& attributes)
	{
		Json result = Json::object();
		for (const auto& value : attributes.BuildSnapshot()) result[std::string{ value.id.GetName() }] = { value.baseValue, value.currentValue, value.minValue, value.maxValue };
		return result;
	}

	Json Snapshot(DummyEnemy& actor)
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
		return {
			{ "components", components }, { "owner", Attributes(actor.GetAbilitySystemComponent().GetAttributes()) }, { "shipAttributes", Attributes(actor.GetShipRuntime().GetAttributes()) },
			{ "abilities", abilities }, { "lights", lights }, { "ship", actor.GetShip() },
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

	std::shared_ptr<DummyEnemy> Direct(World& world)
	{
		const auto* ship = content::ShipContentCatalog::FindById(DummyShipId);
		return ship ? world.SpawnActor<DummyEnemy>(*ship).lock() : nullptr;
	}

	std::size_t LiveActors(World& world)
	{
		std::size_t count = 0;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}

	void Compare(DummyEnemy& direct, DummyEnemy& prefab, const std::string& label)
	{
		const auto expected = Snapshot(direct), actual = Snapshot(prefab);
		for (const auto& field : expected.items()) Check(field.value() == actual.at(field.key()), label + " " + field.key(), actual.at(field.key()));
	}

	void CheckSparse(const Json& document, const std::string& shipId)
	{
		Check(document.at("base") == Json{ { "type", "DummyEnemy" } } && document.at("properties") == Json{ { "Ship", shipId } }, "dummy sparse base and selector", document);
		for (const auto& field : document.items())
			Check(field.key() == "format" || field.key() == "version" || field.key() == "name" || field.key() == "base" || field.key() == "properties", "dummy no snapshot field " + field.key());
	}

	void Equivalence(EngineContext& engine, const std::filesystem::path& root)
	{
		std::string reason;
		Check(content::ShipContentCatalog::FindById(DummyShipId) != nullptr, "Dummy source ship is present");
		CheckSparse(Read(root / DummyPrefabPath), DummyShipId);
		Check(ValidateDummyPrefab(engine, root, DummyPrefabPath, DummyShipId, &reason), "Dummy validator accepts", reason);
		Check(!ValidateDummyPrefab(engine, root, DummyPrefabPath, "Ship.Missing.Basic", &reason), "Dummy validator rejects unknown ship", reason);
		{
			EngineContext isolated;
			EngineContext::Scope isolatedScope{ isolated };
			InstallVegaEngineAssets();
			isolated.Assets().SetAssetRootDirectory(root.generic_string() + "/");
			World world{ isolated };
			Check(!SpawnDummyPrefab(world, DummyPrefabPath, DummyShipId, &reason).expired(), "Dummy prefab spawn registers owning context", reason);
		}
		World directWorld{ engine }, prefabWorld{ engine };
		const auto direct = Direct(directWorld), prefab = SpawnDummyPrefab(prefabWorld, DummyPrefabPath, DummyShipId, &reason).lock();
		Check(prefab != nullptr, "Dummy runtime spawn", reason);
		if (!direct || !prefab) return;
		const sf::Vector2f location{ 451.f, 673.f };
		direct->SetActorLocation(location);
		prefab->SetActorLocation(location);
		Compare(*direct, *prefab, "constructed");
		Check(!prefab->IsPhysicsEnabled() && !prefab->GetMovementComponent().HasBegunPlay() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0,
			"Dummy construction starts no gameplay");
		const auto* property = engine.Types().Find("DummyEnemy")->FindProperty("Ship");
		Check(property && property->construction && property->SetValue(prefab.get(), std::string{ DummyShipId }, &reason), "Dummy Ship construction metadata and idempotence");
		Check(property && !property->SetValue(prefab.get(), std::string{ "Ship.Enemy.ApproachGunner.Basic" }, &reason) &&
			reason.find("construction-only") != std::string::npos && prefab->GetShip() == DummyShipId, "Dummy refuses changed Ship", reason);
		const auto saved = SceneSerializer::Save(prefabWorld);
		Check(saved["entities"].size() == 1 && saved["entities"][0].at("prefab") == DummyPrefabPath && !saved["entities"][0].contains("components"), "Dummy sparse scene");
		World reopened{ engine };
		const auto loaded = SceneSerializer::Load(reopened, saved);
		Check(loaded.spawned == 1 && loaded.warnings.empty() && SceneSerializer::Save(reopened) == saved, "Dummy scene reopens", loaded.warnings);
		PrefabLibrary library{ engine, root };
		auto sparse = library.DiffType(SceneSerializer::SaveEntity(*prefab, true));
		Check(sparse && sparse.warnings.empty(), "Dummy sparse type authoring succeeds", sparse.warnings);
		if (sparse)
		{
			sparse.entity["type"] = "DummyEnemy";
			CheckSparse(PrefabLibrary::Document(sparse.entity, "Dummy Standard"), DummyShipId);
		}
		direct->BeginPlayInternal();
		prefab->BeginPlayInternal();
		Compare(*direct, *prefab, "begun");
		direct->Tick(0.016f);
		prefab->Tick(0.016f);
		Compare(*direct, *prefab, "tick");
		prefab->Destroy();
		prefabWorld.TickInternal(0.f);
		prefabWorld.GetTimerManager().UpdateTimer(0.f);
		Check(LiveActors(prefabWorld) == 0 && prefabWorld.GetActorCountIncludingPending() == 0 && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0, "Dummy destruction reclaims actors and timers");
		direct->Destroy();
		directWorld.TickInternal(0.f);
	}

	void HeavyEquivalence(EngineContext& engine, const std::filesystem::path& root)
	{
		constexpr const char* HeavyShipId = "Ship.Enemy.HeavyDummy.Basic";
		constexpr const char* HeavyPrefabPath = "prefabs/dummies/dummy_heavy.prefab.json";
		std::string reason;
		Check(content::ShipContentCatalog::FindById(HeavyShipId) != nullptr, "Heavy dummy source ship is present");
		CheckSparse(Read(root / HeavyPrefabPath), HeavyShipId);
		Check(ValidateDummyPrefab(engine, root, HeavyPrefabPath, HeavyShipId, &reason), "Heavy dummy validator accepts", reason);
		Check(!ValidateDummyPrefab(engine, root, HeavyPrefabPath, "Ship.Missing.Basic", &reason), "Heavy dummy validator rejects unknown ship", reason);
		Check(!ValidateDummyPrefab(engine, root, HeavyPrefabPath, DummyShipId, &reason), "Heavy dummy validator rejects a mismatched selector", reason);
		// The old arena path built the heavy dummy in code from RangeKeeper plus overrides;
		// the catalog entry must reproduce every ShipRuntime-exposed attribute exactly.
		ShipDefinition overrideDefinition = *content::ShipContentCatalog::FindById(DummyShipId);
		overrideDefinition.health = 9000.f;
		overrideDefinition.baseOwnerAttributes.push_back({ OwnerAttributeIds::Armor, 40.f });
		overrideDefinition.speed = {};
		overrideDefinition.primaryWeaponId = "Weapon.Projectile.FighterRapidLaser.Basic";
		World overrideWorld{ engine }, catalogWorld{ engine }, prefabWorld{ engine };
		const auto overrideDummy = overrideWorld.SpawnActor<DummyEnemy>(overrideDefinition).lock();
		const auto catalogDummy = catalogWorld.SpawnActor<DummyEnemy>(*content::ShipContentCatalog::FindById(HeavyShipId)).lock();
		const auto prefabDummy = SpawnDummyPrefab(prefabWorld, HeavyPrefabPath, HeavyShipId, &reason).lock();
		Check(overrideDummy && catalogDummy && prefabDummy != nullptr, "Heavy override, catalog and prefab dummies spawn", reason);
		if (!overrideDummy || !catalogDummy || !prefabDummy) return;
		const sf::Vector2f location{ 2100.f, 1950.f };
		overrideDummy->SetActorLocation(location);
		catalogDummy->SetActorLocation(location);
		prefabDummy->SetActorLocation(location);
		const auto compare = [&](DummyEnemy& expectedActor, DummyEnemy& actualActor, const std::string& stage)
		{
			const auto expected = Snapshot(expectedActor), actual = Snapshot(actualActor);
			for (const auto& field : expected.items())
			{
				// The catalog id legitimately differs from the code-built RangeKeeper source.
				if (field.key() == "ship") continue;
				Check(field.value() == actual.at(field.key()), "Heavy " + stage + " " + field.key(), actual.at(field.key()));
			}
		};
		compare(*overrideDummy, *catalogDummy, "override vs catalog");
		compare(*catalogDummy, *prefabDummy, "catalog vs prefab");
		overrideDummy->BeginPlayInternal();
		catalogDummy->BeginPlayInternal();
		prefabDummy->BeginPlayInternal();
		compare(*overrideDummy, *catalogDummy, "override vs catalog begun");
		overrideDummy->Tick(0.016f);
		catalogDummy->Tick(0.016f);
		prefabDummy->Tick(0.016f);
		compare(*overrideDummy, *catalogDummy, "override vs catalog tick");
		compare(*catalogDummy, *prefabDummy, "catalog vs prefab tick");
	}

	void Validation(EngineContext& engine, const std::filesystem::path& root)
	{
		const auto valid = Read(root / DummyPrefabPath);
		const std::string path = "prefabs/dummies/validation.prefab.json";
		const auto reject = [&](const Json& value, const std::string& name, const std::string& expected = DummyShipId)
		{
			Write(root / path, value);
			std::string reason;
			Check(!ValidateDummyPrefab(engine, root, path, expected, &reason) && !reason.empty(), "Reject " + name, reason);
			World world{ engine };
			Check(SpawnDummyPrefab(world, path, expected, &reason).expired() && world.GetActorCountIncludingPending() == 0, "Rejected " + name + " creates no actors", reason);
		};
		auto candidate = valid; candidate["properties"].erase("Ship"); reject(candidate, "missing selector");
		candidate = valid; candidate["properties"]["Ship"] = "Ship.Enemy.ApproachGunner.Basic"; reject(candidate, "different selector");
		candidate = valid; candidate["properties"]["Ship"] = 3; reject(candidate, "non-string selector");
		candidate = valid; candidate["properties"]["Ship"] = "Ship.Missing.Basic"; reject(candidate, "unknown selector");
		candidate = valid; candidate["base"]["type"] = "EnemyActor"; reject(candidate, "wrong type");
		candidate = valid; candidate["base"]["prefab"] = DummyPrefabPath; reject(candidate, "ambiguous base");
		for (const auto& name : { "Render Enabled", "Location", "Health", "XP Reward" })
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
		for (const auto& badPath : { std::string{ "../outside.prefab.json" }, std::string{ "prefabs/dummies/missing.prefab.json" }, (root / DummyPrefabPath).generic_string() })
			Check(!ValidateDummyPrefab(engine, root, badPath, DummyShipId, &reason) && !reason.empty(), "Reject invalid asset path " + badPath, reason);
		candidate = valid; candidate["base"] = { { "prefab", DummyPrefabPath } }; candidate.erase("properties");
		Write(root / path, candidate);
		Check(ValidateDummyPrefab(engine, root, path, DummyShipId, &reason), "Accept inherited selector", reason);
	}

	void EditorPostcheck(EngineContext& engine, const std::filesystem::path& root, const std::filesystem::path& scenePath)
	{
		script::ScriptRuntime::Get(engine).SetRoot(root);
		script::RegisterScriptTypes(engine);
		const auto scene = Read(scenePath);
		World reopened{ engine };
		const auto loaded = SceneSerializer::Load(reopened, scene);
		Check(loaded.warnings.empty(), "Editor scene loads without warnings", loaded.warnings);
		CheckSparse(Read(root / DummyPrefabPath), DummyShipId);
		std::string reason;
		Check(ValidateDummyPrefab(engine, root, DummyPrefabPath, DummyShipId, &reason), "Dummy editor prefab validates", reason);
		World directWorld{ engine }, prefabWorld{ engine };
		const auto direct = Direct(directWorld), prefab = SpawnDummyPrefab(prefabWorld, DummyPrefabPath, DummyShipId, &reason).lock();
		Check(prefab != nullptr, "Dummy editor prefab spawns", reason);
		if (direct && prefab) Compare(*direct, *prefab, "editor");
		int instances = 0;
		for (const auto& entity : scene.at("entities")) if (entity.value("prefab", std::string{}) == DummyPrefabPath)
		{
			++instances;
			const auto properties = entity.value("properties", Json::object());
			Check(!entity.contains("components") && !entity.contains("scripts") && properties.size() <= 1, "Dummy editor scene remains sparse", entity);
			for (const auto& property : properties.items()) Check(property.key() == "Location", "Dummy editor scene authors only placement");
		}
		Check(instances == 1, "Dummy editor scene retains one instance");
		Write(root / "dummy-authoring-check.json", { { "checks", checks }, { "failures", failures } });
	}
}

int RunTests(int argc, char** argv)
{
	using namespace vega;
	EngineContext engine;
	EngineContext::Scope scope{ engine };
	InstallVegaEngineAssets();
	const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	const auto artifacts = std::filesystem::path{ DUMMY_PREFAB_ARTIFACT_DIR };
	std::filesystem::create_directories(artifacts);
	const bool editorPostcheck = argc == 4 && std::string{ argv[1] } == "--accept-sparse-root";
	const auto root = editorPostcheck ? std::filesystem::path{ argv[2] } : artifacts / "assets";
	if (!editorPostcheck) std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	engine.Assets().SetAssetRootDirectory(root.generic_string() + "/");
	RegisterGameTypes();
	if (!GameContentBootstrap::Register()) { std::cerr << "Dummy prefab bootstrap failed\n"; return 1; }
	Check(content::ShipContentCatalog::IsLoaded(), "Ship catalog is loaded");
	if (editorPostcheck) EditorPostcheck(engine, root, argv[3]);
	else
	{
		{
			World world{ engine };
			const auto* type = engine.Types().Find("DummyEnemy");
			const auto actor = std::dynamic_pointer_cast<DummyEnemy>(type->create(world));
			Check(type->spawnable && actor && actor->GetShip() == DummyShipId && !actor->IsPhysicsEnabled(), "Editor class creation constructs the neutral default dummy");
		}
		Equivalence(engine, root);
		HeavyEquivalence(engine, root);
		Validation(engine, root);
		Write(artifacts / "dummy-prefab-results.json", { { "checks", checks }, { "failures", failures } });
	}
	std::cout << checks.size() << " dummy prefab E2E checks; " << failures << " failures\n";
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
	catch (const std::exception& error) { std::cerr << "Dummy prefab E2E exception: " << error.what() << '\n' << checks.dump(2, ' ', false, Json::error_handler_t::replace) << '\n'; return 1; }
}
