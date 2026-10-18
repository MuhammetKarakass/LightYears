#include "gameFramework/LightYearsAssets.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "level/SpriteActor.h"
#include "player/PlayerManager.h"
#include "reflection/GameReflection.h"
#include <framework/AssetManager.h>
#include <framework/ParallaxComponent.h>
#include <framework/SpriteComponent.h>
#include <framework/World.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/scene/Scene.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
	using Json = nlohmann::json;
	Json checks = Json::array();
	int failures = 0;

	void Check(bool passed, const std::string& name, Json actual = nullptr)
	{
		checks.push_back({ { "name", name }, { "passed", passed }, { "actual", std::move(actual) } });
		if (!passed) { ++failures; std::cerr << name << '\n'; }
	}

	std::size_t LiveActors(vega::World& world)
	{
		std::size_t count = 0;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<vega::Actor>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++count;
		return count;
	}

	// Places one prefab instance exactly as a scene would, through the engine
	// prefab API, and returns the realized sprite actor.
	vega::shared_ptr<vega::SpriteActor> SpawnInstance(vega::World& world, const std::string& path, const std::string& guid, vega::SceneLoadResult& load)
	{
		const Json scene{ { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() },
			{ "entities", Json::array({ Json{ { "guid", guid }, { "name", path }, { "prefab", path } } }) } };
		load = vega::SceneSerializer::Load(world, scene);
		const auto actors = world.GetActorsByTypeIncludingPending<vega::SpriteActor>();
		return actors.empty() ? nullptr : actors.front().lock();
	}
}

int main()
{
	using namespace vega;
	EngineContext context;
	EngineContext::Scope scope{ context };
	InstallVegaEngineAssets();
	const auto assets = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	context.Assets().SetAssetRootDirectory(assets.generic_string() + "/");
	const auto artifacts = std::filesystem::path{ PLANET_PREFAB_ARTIFACT_DIR };
	std::filesystem::create_directories(artifacts);
	RegisterGameTypes();
	Check(GameContentBootstrap::Register(), "Content bootstrap failed");

	std::vector<std::string> prefabPaths;
	for (const auto& entry : std::filesystem::directory_iterator{ assets / "prefabs/environment" })
		if (entry.path().filename().string().find(".prefab.json") != std::string::npos)
			prefabPaths.push_back("prefabs/environment/" + entry.path().filename().string());
	std::sort(prefabPaths.begin(), prefabPaths.end());
	Check(prefabPaths.size() == 5, "Expected the four planet prefabs and one variant", Json(prefabPaths));

	// A level world already carrying one planet prefab is the state that must
	// survive every later prefab spawn unchanged.
	World level{ context };
	SceneLoadResult levelLoad;
	const auto levelPlanet = SpawnInstance(level, "prefabs/environment/planet_blue.prefab.json", "level-planet", levelLoad);
	const auto levelSentinel = level.SpawnActor<SpriteActor>().lock();
	Check(levelLoad.spawned == 1 && levelLoad.warnings.empty() && levelPlanet && levelSentinel, "Level world builds without warnings");
	const auto levelBefore = SceneSerializer::Save(level);
	const auto levelActors = LiveActors(level);

	for (std::size_t index = 0; index < prefabPaths.size(); ++index)
	{
		const auto& path = prefabPaths[index];
		const auto resolved = context.Prefabs().Resolve(path);
		Check(resolved && resolved.warnings.empty(), "Prefab resolves without warnings: " + path, Json(resolved.warnings));

		World world{ context };
		SceneLoadResult load;
		const auto actor = SpawnInstance(world, path, "planet-" + std::to_string(index), load);
		Check(load.spawned == 1 && load.warnings.empty() && actor != nullptr, "Prefab spawns exactly one sprite actor: " + path, Json(load.warnings));
		const auto* sprite = actor ? actor->FindComponent<SpriteComponent>() : nullptr;
		const auto* parallax = actor ? actor->FindComponent<ParallaxComponent>() : nullptr;
		Check(sprite && sprite->HasSprite(), "Spawned prefab has a loaded sprite: " + path);
		Check(parallax != nullptr, "Spawned prefab has a ParallaxComponent: " + path);
		Check(LiveActors(world) == 1 && world.GetTimerManager().GetTraversalTimerCount() == 0 && world.unresolvedPrefabs.empty(),
			"Prefab spawn leaves no extra actors, timers or unresolved data: " + path);
	}

	// The variant inherits the base planet presentation and overrides Depth and Scale.
	World baseWorld{ context }, variantWorld{ context };
	SceneLoadResult baseLoad, variantLoad;
	const auto base = SpawnInstance(baseWorld, "prefabs/environment/planet_blue.prefab.json", "base", baseLoad);
	const auto variant = SpawnInstance(variantWorld, "prefabs/environment/planet_blue_large.prefab.json", "variant", variantLoad);
	const auto* baseParallax = base ? base->FindComponent<ParallaxComponent>() : nullptr;
	const auto* variantParallax = variant ? variant->FindComponent<ParallaxComponent>() : nullptr;
	const auto* baseSprite = base ? base->FindComponent<SpriteComponent>() : nullptr;
	const auto* variantSprite = variant ? variant->FindComponent<SpriteComponent>() : nullptr;
	Check(variant && variantLoad.warnings.empty(), "Variant spawns without warnings");
	Check(baseParallax && variantParallax && baseParallax->GetDepth() == 0.3f && variantParallax->GetDepth() == 0.5f,
		"Variant overrides Depth over the inherited default");
	Check(variantParallax && variantParallax->GetCanRotate() && variantParallax->GetRotationSpeed() == 6.f,
		"Variant inherits rotation fields from the base prefab");
	Check(baseSprite && variantSprite && baseSprite->GetTexturePath() == variantSprite->GetTexturePath() &&
		variantSprite->GetTexturePath() == "SpaceShooterRedux/PNG/Planets/Planet2.png", "Variant inherits the base texture");
	Check(variantSprite && variantSprite->GetScale() == sf::Vector2f{ 1.6f, 1.6f }, "Variant overrides the sprite scale");

	// Spawning prefabs anywhere must not touch the level world or start gameplay.
	Check(SceneSerializer::Save(level) == levelBefore && LiveActors(level) == levelActors, "Prefab spawns leave the level world unchanged");
	Check(PlayerManager::GetPlayerManager().GetPlayers().empty(), "Prefab spawns start no Player");

	std::ofstream{ artifacts / "planet-prefab-results.json" } << Json{ { "failures", failures }, { "checks", checks } }.dump(2) << '\n';
	std::cout << checks.size() << " planet prefab E2E checks; " << failures << " failures\n";
	return failures ? 1 : 0;
}
