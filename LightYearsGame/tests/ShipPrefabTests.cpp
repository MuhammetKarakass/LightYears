#include "gameFramework/LightYearsAssets.h"
#include "gameConfigs/GameplayTags.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ShipPrefabSpawner.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "reflection/GameReflection.h"
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
#include <utility>
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif

namespace
{
	using Json = nlohmann::json;
	Json checks = Json::array();
	int failures = 0;
	std::string checkPrefix;

	class EditWorld final : public vega::World
	{
	public:
		using World::World;
		bool IsEditMode() const override { return true; }
	};

	void Check(bool passed, const std::string& name, Json actual = nullptr)
	{
		checks.push_back({ { "name", checkPrefix + name }, { "passed", passed }, { "actual", std::move(actual) } });
		if (!passed) { ++failures; std::cerr << checkPrefix << name << '\n'; }
	}

	Json Attributes(const sas::AttributeSystem& attributes)
	{
		Json result = Json::object();
		for (const auto& value : attributes.BuildSnapshot()) result[std::string{ value.id.GetName() }] = { value.baseValue, value.currentValue, value.minValue, value.maxValue };
		return result;
	}

	Json Abilities(vega::PlayerSpaceShip& ship)
	{
		Json result = Json::object();
		for (const auto& snapshot : ship.GetAbilitySystemComponent().BuildAbilitySnapshots())
		{
			const auto* ability = ship.GetAbilitySystemComponent().GetAbility(snapshot.handle);
			auto& entry = result[snapshot.abilityId];
			if (entry.is_null()) entry = Json::object();
			entry["count"] = entry.value("count", 0) + 1;
			result[snapshot.abilityId]["level"] = ability ? ability->GetLevel() : 0;
		}
		return result;
	}

	Json Components(vega::PlayerSpaceShip& ship)
	{
		Json result = Json::object();
		ship.ForEachComponent([&](vega::ActorComponent& component)
		{
			if (dynamic_cast<vega::SceneEntityComponent*>(&component)) return;
			const auto* type = ship.GetEngineContext().Types().FindMostDerived(component);
			const auto name = type ? type->name : typeid(component).name();
			result[name] = result.value(name, 0) + 1;
		});
		return result;
	}

	Json EngineLights(vega::PlayerSpaceShip& ship)
	{
		Json result = Json::object();
		auto* lights = ship.FindComponent<vega::LightComponent>();
		if (!lights) return result;
		for (const auto& base : { vega::GameTags::Ship::Engine_Left, vega::GameTags::Ship::Engine_Right })
			for (const auto& tag : lights->GetLightsByBaseTag(base))
			{
				const auto& light = *lights->GetLightData(tag);
				result[tag.ToString()] = { { "offset", { light.offset.x, light.offset.y } }, { "color", { light.color.r, light.color.g, light.color.b, light.color.a } },
					{ "size", { light.size.x, light.size.y } }, { "intensity", light.intensity }, { "stretch", light.shouldStretch }, { "trail", light.useComplexTrail },
					{ "taper", light.taperAmount }, { "softness", light.edgeSoftness }, { "roundness", light.shapeRoundness }, { "enabled", light.isEnabled } };
			}
		return result;
	}

	void Compare(vega::PlayerSpaceShip& direct, vega::PlayerSpaceShip& prefab, const char* phase)
	{
		Check(Components(direct) == Components(prefab), std::string{ phase } + " component set", Components(prefab));
		Check(EngineLights(direct) == EngineLights(prefab) && EngineLights(prefab).size() == 2, std::string{ phase } + " engine mounts and light style", EngineLights(prefab));
		const auto* directSprite = direct.FindComponent<vega::SpriteComponent>();
		const auto* prefabSprite = prefab.FindComponent<vega::SpriteComponent>();
		Check(directSprite && prefabSprite && directSprite->GetTexturePath() == prefabSprite->GetTexturePath() && directSprite->GetScale() == prefabSprite->GetScale(), std::string{ phase } + " sprite texture and scale");
		Check(Attributes(direct.GetAbilitySystemComponent().GetAttributes()) == Attributes(prefab.GetAbilitySystemComponent().GetAttributes()), std::string{ phase } + " owner attributes");
		Check(Attributes(direct.GetShipRuntime().GetAttributes()) == Attributes(prefab.GetShipRuntime().GetAttributes()), std::string{ phase } + " ship attributes");
		Check(Abilities(direct) == Abilities(prefab), std::string{ phase } + " exact ability grants and levels", Abilities(prefab));
		const auto& a = direct.GetMovementComponent().GetAttributes();
		const auto& b = prefab.GetMovementComponent().GetAttributes();
		Check(a.forwardThrust.currentValue == b.forwardThrust.currentValue && a.reverseThrust.currentValue == b.reverseThrust.currentValue &&
			a.strafeThrust.currentValue == b.strafeThrust.currentValue && a.maxSpeed.currentValue == b.maxSpeed.currentValue &&
			a.angularTurnSpeed.currentValue == b.angularTurnSpeed.currentValue && a.angularTurnResponsiveness.currentValue == b.angularTurnResponsiveness.currentValue &&
			a.linearDamping.currentValue == b.linearDamping.currentValue && a.inputResponsiveness.currentValue == b.inputResponsiveness.currentValue &&
			a.mouseAimDeadZone.currentValue == b.mouseAimDeadZone.currentValue && direct.GetSpeed() == prefab.GetSpeed() &&
			direct.GetUseScreenClamp() == prefab.GetUseScreenClamp() && direct.GetMovementMode() == prefab.GetMovementMode(), std::string{ phase } + " movement");
		Check(direct.GetHealthComponent().GetHealth() == prefab.GetHealthComponent().GetHealth() && direct.GetShieldComponent().GetShield() == prefab.GetShieldComponent().GetShield() &&
			direct.GetEnergyComponent().GetEnergy() == prefab.GetEnergyComponent().GetEnergy(), std::string{ phase } + " component resources");
		Check(a.horizontalRatingToThrust == b.horizontalRatingToThrust && a.verticalRatingToThrust == b.verticalRatingToThrust && a.ratingToMaxSpeed == b.ratingToMaxSpeed && a.movementRatingDiminishingScale == b.movementRatingDiminishingScale, std::string{ phase } + " movement rating conversion");
		Check(direct.GetModuleApplier().HasApplied() == prefab.GetModuleApplier().HasApplied() && direct.GetModuleApplier().GetOwnedHandleCount() == prefab.GetModuleApplier().GetOwnedHandleCount() &&
			direct.GetEnabledModuleSystems() == prefab.GetEnabledModuleSystems(), std::string{ phase } + " module state");
	}

	std::size_t LiveActors(vega::World& world)
	{
		std::size_t count = 0;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<vega::Actor>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}
}

int RunTests(int argc, char** argv)
{
	using namespace vega;
	EngineContext context;
	EngineContext::Scope scope{ context };
	if (argc == 4 && std::string{ argv[1] } == "--accept-sparse-root")
	{
		const std::filesystem::path root{ argv[2] };
		const bool expectedRender = Json::parse(argv[3]).get<bool>();
		Json document;
		std::ifstream{ root / "prefabs/ships/player/fighter.prefab.json" } >> document;
		Check(document.value("properties", Json::object()) == Json{ { "Ship", DefaultPlayerShipId }, { "Render Enabled", expectedRender } }, "Saved prefab contains exactly Ship and intentional Render Enabled override", document);
		Check(document.value("base", Json::object()) == Json{ { "prefab", "prefabs/ships/player/player_ship.prefab.json" } }, "Editor Fighter retains the shared base prefab link");
		Check(!document.contains("components"), "Saved prefab contains no constructor component snapshots");
		std::string reason;
		const bool valid = ValidateShipPrefab(context, root, "prefabs/ships/player/fighter.prefab.json", DefaultPlayerShipId, &reason);
		Check(valid, "Game validator accepts editor-authored prefab", reason);
		InstallVegaEngineAssets();
		context.Assets().SetAssetRootDirectory(root.generic_string() + "/");
		RegisterGameTypes();
		const bool bootstrapped = GameContentBootstrap::Register();
		Check(bootstrapped, "Editor asset copy passes game content bootstrap");
		Json interceptorDocument;
		std::ifstream{ root / "prefabs/ships/player/interceptor.prefab.json" } >> interceptorDocument;
		Check(interceptorDocument.value("properties", Json::object()) == Json{ { "Ship", "Ship.Player.Interceptor.Basic" }, { "Render Enabled", false } } &&
			interceptorDocument.value("base", Json::object()) == Json{ { "prefab", "prefabs/ships/player/player_ship.prefab.json" } } && !interceptorDocument.contains("components"), "Editor Interceptor remains a sparse selected variant", interceptorDocument);
		Check(ValidateShipPrefab(context, root, "prefabs/ships/player/interceptor.prefab.json", "Ship.Player.Interceptor.Basic", &reason), "Game validator accepts editor Interceptor variant", reason);
		if (bootstrapped)
		{
			World directWorld{ context }, prefabWorld{ context };
			const auto direct = directWorld.SpawnActor<PlayerSpaceShip>(*content::ShipContentCatalog::FindById("Ship.Player.Interceptor.Basic")).lock();
			const auto prefab = SpawnShipPrefab(prefabWorld, "prefabs/ships/player/interceptor.prefab.json", "Ship.Player.Interceptor.Basic", &reason).lock();
			Check(prefab && prefab->GetShip() == "Ship.Player.Interceptor.Basic" && !prefab->IsRenderEnabled(), "Editor Interceptor reopens with its construction choice and visibility", reason);
			if (prefab) Compare(*direct, *prefab, "Editor Interceptor");
		}
		if (valid && bootstrapped)
		{
			World directWorld{ context }, prefabWorld{ context };
			const auto direct = directWorld.SpawnActor<PlayerSpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			const auto prefab = SpawnShipPrefab(prefabWorld, "prefabs/ships/player/fighter.prefab.json", DefaultPlayerShipId, &reason).lock();
			Check(prefab != nullptr, "Game spawner accepts editor-authored prefab", reason);
			if (prefab)
			{
				Compare(*direct, *prefab, "Editor-authored");
				Check(prefab->IsRenderEnabled() == expectedRender, "Game spawn preserves intentional Render Enabled override");
			}
		}
		std::ofstream{ root / (expectedRender ? "fighter-authoring-check-true.json" : "fighter-authoring-check-false.json") } << Json{ { "document", document }, { "gameAccepted", valid }, { "reason", reason }, { "checks", checks }, { "failures", failures } }.dump(2) << '\n';
		std::cout << "Sparse Fighter authoring: " << failures << " failures\n";
		return failures ? 1 : 0;
	}
	InstallVegaEngineAssets();
	const auto source = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	const auto artifacts = std::filesystem::path{ SHIP_PREFAB_ARTIFACT_DIR };
	std::filesystem::create_directories(artifacts);
	context.Assets().SetAssetRootDirectory(source.generic_string() + "/");
	RegisterGameTypes();
	if (!GameContentBootstrap::Register()) { std::cerr << "Content bootstrap failed\n"; return 1; }
	const auto& definition = content::ShipContentCatalog::GetPlayerFighterDefinition();
	Check(definition.prefabPath.has_value(), "Fighter selects a prefab");
	if (!definition.prefabPath) return 1;
	const auto path = *definition.prefabPath;
	{
		EngineContext isolated;
		EngineContext::Scope isolatedScope{ isolated };
		InstallVegaEngineAssets();
		isolated.Assets().SetAssetRootDirectory(source.generic_string() + "/");
		World isolatedWorld{ isolated };
		std::string reason;
		Check(!SpawnShipPrefab(isolatedWorld, path, DefaultPlayerShipId, &reason).expired(), "Standalone prefab spawn registers reflection in the owning engine context", reason);
	}
	// Keep fixtures confined to the build tree, including all shipped prefab dependencies.
	std::filesystem::copy(source / "prefabs", artifacts / "prefabs", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	for (const auto& id : content::ShipContentCatalog::GetPlayerShipIds())
	{
		const auto& selected = *content::ShipContentCatalog::FindById(id);
		Check(selected.prefabPath.has_value(), id + " selects a prefab");
		if (!selected.prefabPath) continue;
		std::string diagnostic;
		Check(ValidateShipPrefab(context, source, *selected.prefabPath, id, &diagnostic), id + " validator accepts shipped prefab", diagnostic);
		World directWorld{ context }, prefabWorld{ context };
		const auto direct = directWorld.SpawnActor<PlayerSpaceShip>(selected).lock();
		const auto prefab = SpawnShipPrefab(prefabWorld, *selected.prefabPath, id, &diagnostic).lock();
		Check(prefab && diagnostic.empty(), id + " prefab selection reaches constructor", diagnostic);
		if (!prefab) continue;
		Check(prefab->GetShip() == id && direct->GetShip() == id, id + " prefab retains catalog identity");
		Compare(*direct, *prefab, (id + " constructed").c_str());
		Check(!prefab->IsPhysicsEnabled() && !prefab->GetMovementComponent().HasBegunPlay() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0 &&
			LiveActors(prefabWorld) == 1 && PlayerManager::GetPlayerManager().GetPlayers().empty(), id + " scratch construction has no gameplay side effects");
		const auto saved = SceneSerializer::Save(prefabWorld);
		World reopened{ context };
		Check(SceneSerializer::Load(reopened, saved).warnings.empty() && SceneSerializer::Save(reopened) == saved && saved["entities"].size() == 1 &&
			saved["entities"][0]["prefab"] == *selected.prefabPath && !saved["entities"][0].contains("components"), id + " sparse scene save/reopen", saved);
		const auto reopenedShips = reopened.GetActorsByTypeIncludingPending<PlayerSpaceShip>();
		Check(reopenedShips.size() == 1, id + " reopened exactly one ship");
		if (reopenedShips.size() == 1) if (const auto ship = reopenedShips.front().lock()) Compare(*direct, *ship, (id + " reopened").c_str());
		Player owner{ id };
		const auto rules = owner.BuildModuleRules();
		ModuleLoadout modules;
		Check(rules && modules.SeedDefaults(*rules), id + " seeds its own schematic");
		if (rules)
		{
			Check(modules.GetPlacements().size() == selected.schematic->defaultModules.size(), id + " retains every default module placement");
			direct->GetModuleApplier().Apply(modules, *rules);
			prefab->GetModuleApplier().Apply(modules, *rules);
			Compare(*direct, *prefab, (id + " modules").c_str());
			Check(!prefab->GetModuleApplier().Apply(modules, *rules), id + " module application is idempotent");
		}
		direct->BeginPlayInternal();
		prefab->BeginPlayInternal();
		Compare(*direct, *prefab, (id + " BeginPlay").c_str());
		Check(prefab->IsPhysicsEnabled() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 1, id + " BeginPlay starts one invulnerability timer");
		// A descendant must also override an explicitly selected parent before realization.
		Json parent{ { "format", "vega.prefab" }, { "version", 1 }, { "base", { { "type", "PlayerSpaceShip" } } }, { "properties", { { "Ship", DefaultPlayerShipId } } } };
		Json child{ { "format", "vega.prefab" }, { "version", 1 }, { "base", { { "prefab", "selected-parent.prefab.json" } } }, { "properties", { { "Ship", id } } } };
		std::ofstream{ artifacts / "selected-parent.prefab.json" } << parent.dump(2);
		std::ofstream{ artifacts / "selected-child.prefab.json" } << child.dump(2);
		context.Assets().SetAssetRootDirectory(artifacts.generic_string() + "/");
		World descendantWorld{ context };
		const auto descendant = SpawnShipPrefab(descendantWorld, "selected-child.prefab.json", id, &diagnostic).lock();
		Check(descendant && descendant->GetShip() == id, id + " descendant selector overrides selected Fighter parent", diagnostic);
		World comparisonWorld{ context };
		const auto comparison = comparisonWorld.SpawnActor<PlayerSpaceShip>(selected).lock();
		if (descendant) Compare(*comparison, *descendant, (id + " selected-parent variant").c_str());
		// Reject each forbidden shape for every catalog selection, before any actors can leak.
		parent["properties"]["Ship"] = id;
		for (const std::string fault : { "Ship", "type", "components", "scripts", "children", "Speed", "Render Enabled", "escape" })
		{
			auto invalid = parent;
			if (fault == "Ship") invalid["properties"]["Ship"] = id == DefaultPlayerShipId ? "Ship.Player.Interceptor.Basic" : DefaultPlayerShipId;
			else if (fault == "type") invalid["base"]["type"] = "DummyEnemy";
			else if (fault == "Speed") invalid["properties"]["Speed"] = 1;
			else if (fault == "Render Enabled") invalid["properties"]["Render Enabled"] = "false";
			else if (fault == "escape") invalid["base"] = { { "prefab", "../outside.prefab.json" } };
			else invalid[fault] = Json::array();
			std::ofstream{ artifacts / "all-ships-invalid.prefab.json" } << invalid.dump(2);
			Check(!ValidateShipPrefab(context, artifacts, "all-ships-invalid.prefab.json", id, &diagnostic) && !diagnostic.empty(), id + " validator rejects " + fault, diagnostic);
			const auto count = descendantWorld.GetActorCountIncludingPending();
			Check(SpawnShipPrefab(descendantWorld, "all-ships-invalid.prefab.json", id, &diagnostic).expired() && descendantWorld.GetActorCountIncludingPending() == count &&
				descendantWorld.unresolvedPrefabs.empty() && descendantWorld.GetTimerManager().GetTraversalTimerCount() == 0, id + " rejected " + fault + " leaks no actors or timers");
		}
		// Variant chains must not inherit forbidden parent shapes or escape through a link.
		Json chainParent{ { "format", "vega.prefab" }, { "version", 1 }, { "name", "Chain parent" }, { "base", { { "type", "PlayerSpaceShip" } } }, { "properties", { { "Ship", id } } } };
		Json chainChild{ { "format", "vega.prefab" }, { "version", 1 }, { "name", "Chain child" }, { "base", { { "prefab", "chain-parent.prefab.json" } } }, { "properties", Json::object() } };
		const auto writeChain = [&](const Json& parentDocument)
		{
			std::ofstream{ artifacts / "chain-parent.prefab.json" } << parentDocument.dump(2);
			std::ofstream{ artifacts / "chain-child.prefab.json" } << chainChild.dump(2);
			return ValidateShipPrefab(context, artifacts, "chain-child.prefab.json", id, &diagnostic);
		};
		Check(writeChain(chainParent) && diagnostic.empty(), id + " validator accepts a variant inherited through a sparse chain", diagnostic);
		auto chainSnapshot = chainParent;
		chainSnapshot["components"] = Json::array({ { { "type", "HealthComponent" }, { "properties", { { "Max Health", 1 } } } } });
		Check(!writeChain(chainSnapshot) && !diagnostic.empty(), id + " validator rejects a chain parent with a component snapshot", diagnostic);
		auto chainWrongShip = chainParent;
		chainWrongShip["properties"]["Ship"] = id == DefaultPlayerShipId ? "Ship.Player.Interceptor.Basic" : DefaultPlayerShipId;
		Check(!writeChain(chainWrongShip) && !diagnostic.empty(), id + " validator rejects a chain with a wrong inherited Ship", diagnostic);
		auto chainEscape = chainParent;
		chainEscape["base"] = { { "prefab", "../outside.prefab.json" } };
		Check(!writeChain(chainEscape) && !diagnostic.empty(), id + " validator rejects a chain link escaping the asset root", diagnostic);
		context.Assets().SetAssetRootDirectory(source.generic_string() + "/");
		prefab->Destroy();
		prefabWorld.TickInternal(0.f);
		prefabWorld.GetTimerManager().UpdateTimer(0.f); // Application normally advances timers separately from World.
		Check(LiveActors(prefabWorld) == 0 && prefabWorld.GetActorCountIncludingPending() == 0 && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0, id + " destruction reclaims prefab actors and timers");
	}
	World directWorld{ context }, prefabWorld{ context };
	const auto direct = directWorld.SpawnActor<PlayerSpaceShip>(definition).lock();
	std::string reason;
	const auto prefab = SpawnShipPrefab(prefabWorld, path, DefaultPlayerShipId, &reason).lock();
	Check(prefab && reason.empty(), "Fighter prefab spawns", reason);
	if (!prefab) return 1;
	Check(direct->GetShip() == DefaultPlayerShipId && prefab->GetShip() == DefaultPlayerShipId, "Copied definitions retain catalog identity");
	Compare(*direct, *prefab, "Constructed");
	Check(!prefab->GetMovementComponent().HasBegunPlay() && !prefab->IsPhysicsEnabled() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 0 && LiveActors(prefabWorld) == 1 &&
		!prefab->GetModuleApplier().HasApplied() && prefab->GetModuleApplier().GetGeneratedActorCount() == 0 && PlayerManager::GetPlayerManager().GetPlayers().empty(), "Scratch construction starts no gameplay, timers, modules or Player");
	const auto* shipProperty = context.Types().Find("PlayerSpaceShip")->FindProperty("Ship");
	std::string editError;
	Check(shipProperty->SetValue(prefab.get(), std::string{ DefaultPlayerShipId }, &editError), "Reapplying construction Ship is idempotent");
	Check(!shipProperty->SetValue(prefab.get(), std::string{ "Ship.Player.Interceptor.Basic" }, &editError) && editError.find("construction-only") != std::string::npos && prefab->GetShip() == DefaultPlayerShipId,
		"Changing Ship is refused with an actionable error", editError);
	const auto saved = SceneSerializer::Save(prefabWorld);
	World reopened{ context };
	Check(SceneSerializer::Load(reopened, saved).warnings.empty() && SceneSerializer::Save(reopened) == saved, "Prefab scene save/reopen retains sparse construction data");
	Check(saved["entities"].size() == 1 && saved["entities"][0]["prefab"] == path && !saved["entities"][0].contains("components"), "Saved prefab instance does not copy component catalog values", saved);
	Player moduleOwner;
	const auto rules = moduleOwner.BuildModuleRules();
	ModuleLoadout modules;
	Check(rules && modules.SeedDefaults(*rules), "Seed canonical Fighter module loadout");
	if (rules)
	{
		direct->GetModuleApplier().Apply(modules, *rules);
		prefab->GetModuleApplier().Apply(modules, *rules);
		Compare(*direct, *prefab, "Modules before BeginPlay");
	}
	direct->BeginPlayInternal();
	prefab->BeginPlayInternal();
	Compare(*direct, *prefab, "BeginPlay");
	Check(prefab->IsPhysicsEnabled() && prefabWorld.GetTimerManager().GetTraversalTimerCount() == 1, "BeginPlay starts physics and exactly one invulnerability timer");

	// Every invalid fixture is authored in the build tree, never in source assets.
	context.Assets().SetAssetRootDirectory(artifacts.generic_string() + "/");
	Json document;
	std::ifstream{ source / path } >> document;
	document["base"] = { { "type", "PlayerSpaceShip" } }; // Standalone invalid fixtures keep no source-tree dependencies.
	const auto write = [&](const std::string& name, const Json& value) { std::ofstream{ artifacts / name } << value.dump(2) << '\n'; };
	write("valid.prefab.json", document);
	const auto reject = [&](const std::string& candidate, const std::string& id = DefaultPlayerShipId)
	{
		World failed{ context };
		const auto sentinel = failed.SpawnActor<PlayerSpaceShip>(definition).lock();
		const auto count = failed.GetActorCountIncludingPending();
		const auto missing = failed.unresolvedPrefabs;
		reason.clear();
		Check(SpawnShipPrefab(failed, candidate, id, &reason).expired() && !reason.empty() && LiveActors(failed) == 1 &&
			failed.GetActorCountIncludingPending() == count && !sentinel->GetIsPendingDestroy() && failed.unresolvedPrefabs == missing && failed.GetTimerManager().GetTraversalTimerCount() == 0,
			"Reject without leaked actor or world changes: " + candidate + ":" + id, reason);
	};
	reject("missing.prefab.json");
	std::ofstream{ artifacts / "invalid.prefab.json" } << "{ broken";
	reject("invalid.prefab.json");
	reject("valid.prefab.json", "Ship.Player.Interceptor.Basic");
	reject("valid.prefab.json", "Ship.Player.Unknown.Basic");
	reject("../fighter.prefab.json");
	reject((source / path).generic_string());
	auto changed = document;
	changed["base"]["type"] = "DummyEnemy";
	write("wrong-type.prefab.json", changed); reject("wrong-type.prefab.json");
	changed = document; changed["properties"]["Ship"] = "Ship.Player.Interceptor.Basic";
	write("wrong-ship.prefab.json", changed); reject("wrong-ship.prefab.json");
	changed = document; changed["components"] = Json::array({ { { "type", "HealthComponent" }, { "properties", { { "Max Health", 1 } } } } });
	write("snapshot.prefab.json", changed); reject("snapshot.prefab.json");
	changed = document; changed["children"] = Json::array({ { { "id", "extra-player" }, { "type", "PlayerSpaceShip" } } });
	write("partial.prefab.json", changed); reject("partial.prefab.json");
	changed = document; changed["properties"]["Speed"] = 1;
	write("movement.prefab.json", changed); reject("movement.prefab.json");
	changed = document; changed["properties"]["Render Enabled"] = "not boolean";
	write("bad-property.prefab.json", changed); reject("bad-property.prefab.json");
	write("variant.prefab.json", { { "format", "vega.prefab" }, { "version", 1 }, { "base", { { "prefab", "valid.prefab.json" } } }, { "properties", { { "Render Enabled", false } } } });
	World variantWorld{ context };
	const auto variant = SpawnShipPrefab(variantWorld, "variant.prefab.json", DefaultPlayerShipId, &reason).lock();
	Check(variant && !variant->IsRenderEnabled() && variant->GetHealthComponent().GetHealth() == definition.health, "Variant inherits selection and keeps catalog health");
	write("cycle.prefab.json", { { "format", "vega.prefab" }, { "version", 1 }, { "base", { { "prefab", "cycle.prefab.json" } } } });
	reject("cycle.prefab.json");
	// Depth is bounded independently of cycles; the engine limit is 64 documents.
	for (int index = 0; index < 65; ++index)
	{
		Json link{ { "format", "vega.prefab" }, { "version", 1 }, { "properties", { { "Ship", DefaultPlayerShipId } } } };
		link["base"] = index == 64 ? Json{ { "type", "PlayerSpaceShip" } } : Json{ { "prefab", "depth-" + std::to_string(index + 1) + ".prefab.json" } };
		write("depth-" + std::to_string(index) + ".prefab.json", link);
	}
	reject("depth-0.prefab.json");
	// Inject a serializer diagnostic only in the destination world, after successful
	// scratch resolution, to exercise rollback of an actually created actor.
	EditWorld partialWorld{ context };
	const auto sentinel = partialWorld.SpawnActor<PlayerSpaceShip>(definition).lock();
	ComponentSerializer probe;
	probe.array = "components";
	probe.load = [&](Actor& actor, const Json&, SceneLoadResult& result)
	{
		if (actor.GetWorld() == &partialWorld) result.warnings.push_back("Injected destination composition warning");
	};
	SceneSerializer::RegisterComponentSerializer("ShipPrefabFailureProbe", std::move(probe), context);
	Check(SpawnShipPrefab(partialWorld, "valid.prefab.json", DefaultPlayerShipId, &reason).expired() && reason.find("Injected") != std::string::npos &&
		LiveActors(partialWorld) == 1 && !sentinel->GetIsPendingDestroy() && partialWorld.unresolvedPrefabs.empty(), "Destination warning destroys newly created composition and preserves existing actors", reason);
	context.SceneSerialization().componentSerializers.erase("ShipPrefabFailureProbe");
	partialWorld.TickInternal(0.f);
	Check(partialWorld.GetActorCountIncludingPending() == 1 && partialWorld.GetTimerManager().GetTraversalTimerCount() == 0, "Normal edit-world cleanup reclaims failed pending actors without BeginPlay");

	// A malformed live catalog reference must fail atomically, retaining the last valid catalog.
	Json ships;
	std::ifstream{ source / "content/data/ships.json" } >> ships;
	const auto validShips = ships;
	ships["ships"][0]["prefabPath"] = "wrong-ship.prefab.json";
	write("ships.json", ships);
	Check(!content::ShipContentCatalog::LoadFromFile(artifacts / "ships.json", &reason, true) && content::ShipContentCatalog::FindById(DefaultPlayerShipId) == &definition,
		"Catalog rejects mismatched prefab without replacing valid data", reason);
	// The engine's global cache can contain old constructor snapshots; runtime spawn uses fresh catalog values.
	context.Prefabs().Resolve("valid.prefab.json");
	ships = validShips; ships["ships"][0]["health"] = definition.health + 37.f; ships["ships"][0]["prefabPath"] = "valid.prefab.json";
	write("ships.json", ships);
	Check(content::ShipContentCatalog::LoadFromFile(artifacts / "ships.json", &reason, true), "Reload changed catalog health", reason);
	World freshWorld{ context };
	const auto fresh = SpawnShipPrefab(freshWorld, "valid.prefab.json", DefaultPlayerShipId, &reason).lock();
	Check(fresh && fresh->GetHealthComponent().GetHealth() == definition.health + 37.f, "Cached component snapshots cannot override reloaded catalog health");
	context.Assets().SetAssetRootDirectory(source.generic_string() + "/");
	Check(content::ShipContentCatalog::LoadFromFile(source / "content/data/ships.json", &reason, true), "Restore shipped catalog", reason);

	for (const auto& id : content::ShipContentCatalog::GetPlayerShipIds())
	{
		checkPrefix = id + " respawn: ";
		const auto& selectedDefinition = *content::ShipContentCatalog::FindById(id);
		const auto selectedPath = *selectedDefinition.prefabPath;
		EditWorld runtime{ context };
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		manager.SetSelectedShipId(id);
		Player& player = manager.CreateNewPlayer();
		auto first = player.SpawnSpaceShip(&runtime).lock();
		Check(first && manager.GetPlayers().size() == 1 && player.GetLifeCount() == 2 && LiveActors(runtime) == 1, "Player spawns exactly one prefab ship and consumes one life");
		if (first)
		{
			const auto inventory = player.GetModuleLoadout().GetInventory().size();
			const auto placements = player.GetModuleLoadout().GetPlacements().size();
			const auto grants = Abilities(*first);
			Check(first->FindComponent<SceneEntityComponent>() && first->FindComponent<SceneEntityComponent>()->prefab == selectedPath && first->GetModuleApplier().HasApplied(), "Player uses prefab and applies modules once");
			player.AwardShipXP(selectedDefinition.progressionDefinition.baseXP);
			player.AwardScrap(1000000);
			Check(player.TryPurchaseAbilityLevel(sas::AbilitySlot::Ability1, &reason), "Buy ability level before respawn", reason);
			const auto purchased = first->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1)->GetLevel();
			const auto position = first->GetActorLocation();
			first->Destroy();
			Check(player.GetCurrentSpaceShip().expired(), "Destroy clears Player binding before respawn");
			const auto second = player.SpawnSpaceShip(&runtime).lock();
			Check(second && second != first && manager.GetPlayers().size() == 1 && player.GetLifeCount() == 1 && LiveActors(runtime) == 1, "Respawn retains one Player and one live ship, consumes one life");
			if (second)
			{
				Check(second->FindComponent<SceneEntityComponent>()->prefab == selectedPath && second->GetActorLocation() == position && player.GetShipProgression().GetLevel() == 2,
					"Respawn retains prefab, progression and positioning");
				Check(player.GetModuleLoadout().GetInventory().size() == inventory && player.GetModuleLoadout().GetPlacements().size() == placements && second->GetModuleApplier().HasApplied(), "Respawn does not duplicate module inventory or placements");
				const auto respawnGrants = Abilities(*second);
				bool unique = true;
				for (const auto& entry : respawnGrants.items()) if (entry.value().at("count") != 1) unique = false;
				Check(second->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1)->GetLevel() == purchased && respawnGrants.size() == grants.size() && unique, "Respawn restores purchased levels without double grants", respawnGrants);
				second->Destroy();
			}
		}
		manager.Reset();
		manager.SetSelectedShipId(DefaultPlayerShipId);
		runtime.TickInternal(0.f);
		Check(runtime.GetActorCountIncludingPending() == 0 && runtime.GetTimerManager().GetTraversalTimerCount() == 0 && LiveActors(runtime) == 0, id + " respawn teardown leaves no live ship");
	}
	checkPrefix.clear();
	// Optional prefabPath is still a supported catalog contract, so retain and exercise Player's fallback.
	ships = validShips;
	for (auto& ship : ships["ships"]) ship.erase("prefabPath");
	write("legacy-ships.json", ships);
	Check(content::ShipContentCatalog::LoadFromFile(artifacts / "legacy-ships.json", &reason, true), "Catalog still accepts legacy definitions without prefabPath", reason);
	{
		World legacyWorld{ context };
		Player legacy;
		const auto ship = legacy.SpawnSpaceShip(&legacyWorld).lock();
		Check(ship && !ship->FindComponent<SceneEntityComponent>() && ship->GetShip() == DefaultPlayerShipId && ship->GetModuleApplier().HasApplied(), "Player direct-construction fallback remains reachable");
		if (ship) ship->Destroy();
	}
	Check(content::ShipContentCatalog::LoadFromFile(source / "content/data/ships.json", &reason, true), "Restore prefab catalog after fallback check", reason);
	std::ofstream{ artifacts / "ship-prefab-results.json" } << Json{ { "failures", failures }, { "checks", checks } }.dump(2) << '\n';
	std::cout << checks.size() << " ship prefab E2E checks; " << failures << " failures\n";
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
	catch (const std::exception& error)
	{
		std::cerr << "Ship prefab E2E exception: " << error.what() << '\n' << checks.dump(2) << '\n';
		return 1;
	}
}
