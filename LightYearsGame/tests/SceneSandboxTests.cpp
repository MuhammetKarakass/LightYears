#include <framework/ProjectSettings.h>
#include <framework/EngineContext.h>
#include <framework/CollisionComponent.h>
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/AssetManager.h>
#include <framework/SpriteComponent.h>
#include <framework/reflection/Reflection.h>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include "level/SceneLevel.h"
#include "level/PlayerStart.h"
#include "level/SpriteActor.h"
#include "environment/Asteroid.h"
#include "enemy/DummyEnemy.h"
#include "player/PlayerSpaceShip.h"
#include "player/PlayerManager.h"
#include "reflection/GameReflection.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	class HeadlessSceneLevel : public vega::SceneLevel
	{
	public:
		using SceneLevel::SceneLevel;
		vega::PlayerRespawnDefinition Respawn() const { return CreatePlayerRespawnDefinition(); }
	protected:
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	vega::InstallVegaEngineAssets();
	using namespace vega;
	using Json = nlohmann::json;
	const auto root = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	vega::EngineContext::Current().Assets().SetAssetRootDirectory(root.generic_string() + "/");
	vega::script::SetScriptRoot(root);
	vega::script::RegisterScriptTypes();
	RegisterGameTypes();
	Check(GameContentBootstrap::Register(), "Game content bootstrap failed");
	ProjectSettings::Load(root / "vega.project.json").ApplyCollisionLayers(engineContext);
	std::ifstream input{ root / "scenes/sandbox.scene.json" };
	Json scene;
	input >> scene;
	World world{ nullptr };
	const auto result = SceneSerializer::Load(world, scene);
	for (const auto& warning : result.warnings) std::cerr << warning << '\n';
	Check(result.spawned == 7 && result.warnings.empty(), "Sandbox must load seven entities without warnings");
	Check(world.GetActorsByTypeIncludingPending<PlayerStart>().size() == 1 &&
		world.GetActorsByTypeIncludingPending<DummyEnemy>().size() == 3 &&
		world.GetActorsByTypeIncludingPending<Asteroid>().size() == 2 &&
		world.GetActorsByTypeIncludingPending<SpriteActor>().size() == 1, "Sandbox entity counts are incorrect");
	const auto spriteActor = world.GetActorsByTypeIncludingPending<SpriteActor>()[0].lock();
	Check(spriteActor->FindComponent<SpriteComponent>()->GetScale() == sf::Vector2f{ 3.f, 3.f }, "Sprite scale was not applied");
	const auto* script = spriteActor->FindComponent<vega::script::ScriptComponent>();
	Check(script && script->ScriptPath() == "scripts/spin.lua" && std::get<float>(script->Overrides().at("speed")) == 45.f, "Script path/overrides were not staged");
	const auto saved = SceneSerializer::Save(world, scene["settings"]);
	Check(saved == scene, "Shipped sandbox must satisfy Save(Load(scene)) == scene");
	std::ofstream canonical{ std::filesystem::path{ SCENE_TEST_ARTIFACT_DIR } / "sandbox.scene.json" };
	canonical << saved.dump(1, '\t') << '\n';
	canonical.flush();
	std::ifstream originalBytes{ root / "scenes/sandbox.scene.json", std::ios::binary };
	const std::string authored{ std::istreambuf_iterator<char>{ originalBytes }, std::istreambuf_iterator<char>{} };
	std::ifstream savedBytes{ std::filesystem::path{ SCENE_TEST_ARTIFACT_DIR } / "sandbox.scene.json", std::ios::binary };
	const std::string roundTripBytes{ std::istreambuf_iterator<char>{ savedBytes }, std::istreambuf_iterator<char>{} };
	Check(roundTripBytes == authored, "Unchanged LightYears sandbox bytes differ from the authored scene");
	{
		World labelled{ engineContext };
		auto fixture = labelled.SpawnActor<SpriteActor>().lock();
		fixture->AddComponent<SceneEntityComponent>("labels", "Labels");
		auto& collision = fixture->AddComponent<CollisionComponent>();
		collision.SetCollisionLayer(vega::GameCollision::Player);
		collision.SetCollisionMask(static_cast<CollisionLayer>(255));
		const auto encoded = SceneSerializer::Save(labelled);
		bool found = false;
		for (const auto& component : encoded["entities"][0]["components"])
			if (component["type"] == "CollisionComponent")
			{
				found = true;
				Check(component["properties"]["Collision Mask"] == Json::array({ "Player", "PlayerBullet", "Enemy", "EnemyBullet",
					"Powerup", "Environment", "RelayProjectile", "FriendlySummon" }), "LightYears changed legacy channel labels or bit order");
			}
		Check(found, "LightYears collision fixture was not saved");
		World restored{ engineContext };
		Check(SceneSerializer::Load(restored, encoded).warnings.empty() && SceneSerializer::Save(restored).dump() == encoded.dump(),
			"LightYears nonzero flags did not round-trip byte-identically");
		// No prefab files ship with LightYears yet; exercise its full channel set
		// through the same document/realization path used by the prefab editor.
		const auto prefab = PrefabLibrary::Document(SceneSerializer::SaveSubtree(*fixture, true), "CollisionLabels");
		const auto prefabRoot = std::filesystem::path{ SCENE_TEST_ARTIFACT_DIR };
		std::ofstream{ prefabRoot / "CollisionLabels.prefab.json", std::ios::binary } << prefab.dump(1, '\t') << '\n';
		engineContext.Prefabs().SetRoot(prefabRoot);
		const auto resolved = engineContext.Prefabs().Resolve("CollisionLabels.prefab.json");
		Check(resolved && resolved.warnings.empty(), "LightYears collision prefab failed to resolve");
		if (resolved)
		{
			const auto reopened = PrefabLibrary::Document(resolved.entity, "CollisionLabels");
			std::ifstream prefabBytes{ prefabRoot / "CollisionLabels.prefab.json", std::ios::binary };
			const std::string original{ std::istreambuf_iterator<char>{ prefabBytes }, std::istreambuf_iterator<char>{} };
			Check(reopened.dump(1, '\t') + '\n' == original, "LightYears collision prefab bytes changed on round trip");
		}
	}
	World roundTrip{ nullptr };
	Check(SceneSerializer::Load(roundTrip, saved).warnings.empty() && SceneSerializer::Save(roundTrip, scene["settings"]) == saved, "Game scene round trip changed supported data");
	const auto copiedSprite = roundTrip.GetActorsByTypeIncludingPending<SpriteActor>()[0].lock();
	copiedSprite->BeginPlayInternal();
	Check(copiedSprite->FindComponent<vega::script::ScriptComponent>()->LastError().empty(), "Staged script failed on BeginPlay");
	copiedSprite->TickInternal(1.f);
	Check(copiedSprite->GetActorRotation() == 45.f, "Script update did not use the authored override");

	// Ship edits retain the requested construction data; the next load rebuilds
	// the actual ship from that catalog definition instead of mutating combat.
	const auto dummy = world.GetActorsByTypeIncludingPending<DummyEnemy>()[0].lock();
	const auto& shipIds = content::ShipContentCatalog::GetPlayerShipIds();
	Check(!shipIds.empty(), "Ship catalog is empty");
	const auto* shipProperty = engineContext.Types().Find("DummyEnemy")->FindProperty("Ship");
	Check(shipProperty->SetValue(dummy.get(), shipIds[0]), "Dummy construction-only Ship edit failed");
	World edited{ nullptr };
	auto editedData = SceneSerializer::Save(world, scene["settings"]);
	// Explicit component overrides take precedence over a selected ship's
	// defaults. Clear them here to inspect the new constructor's texture.
	editedData["entities"][1]["components"] = Json::array();
	Check(SceneSerializer::Load(edited, editedData).warnings.empty(), "Edited dummy scene failed to load");
	const auto editedDummy = edited.GetActorsByTypeIncludingPending<DummyEnemy>()[0].lock();
	Check(editedDummy->FindComponent<SceneEntityComponent>()->spawnData["Ship"] == shipIds[0], "Dummy Ship construction data was lost");
	Check(editedDummy->FindComponent<SpriteComponent>()->GetTexturePath() == content::ShipContentCatalog::FindById(shipIds[0])->texturePath, "Dummy factory ignored authored Ship");

	bool applied = false;
	auto level = std::make_shared<HeadlessSceneLevel>(nullptr, scene, [&](const Json& settings) { applied = settings["routeStop"] == 0; });
	level->BeginPlayInternal();
	Check(applied && level->GetSceneLoadResult().spawned == 7, "SceneLevel did not apply settings/load entities");
	Check(level->Respawn().spawnLocation == sf::Vector2f{ 960.f, 700.f } && !level->Respawn().useScreenClamp, "SceneLevel did not use PlayerStart/arena respawn defaults");
	// Edit promotion must not run asteroid randomization, Lua on_create/update,
	// HUD setup, player spawning, or even explicitly paused-ticking actors.
	auto editScene = scene;
	editScene["entities"][6]["properties"]["Tick When Paused"] = true;
	auto editLevel = std::make_shared<SceneLevel>(nullptr, editScene, SceneLevel::SettingsApplier{}, true);
	PlayerManager::GetPlayerManager().SetSelectedShipId("edit-mode-sentinel");
	editLevel->BeginPlayInternal();
	Check(PlayerManager::GetPlayerManager().GetSelectedShipId() == "edit-mode-sentinel", "Edit mode must not change the global selected ship from settings or PlayerStart");
	const auto editBefore = SceneSerializer::Save(*editLevel, scene["settings"]);
	editLevel->TickInternal(1.f);
	editLevel->SetPaused(false);
	editLevel->TickInternal(1.f);
	Check(editLevel->IsEditMode() && editLevel->IsPaused(), "Edit world must remain paused");
	Check(editLevel->GetActorsByType<PlayerSpaceShip>().empty() && editLevel->GetActorsByType<Actor>().size() == 7, "Edit world must contain only authored entities");
	Check(SceneSerializer::Save(*editLevel, scene["settings"]) == editBefore, "Edit ticks must preserve all authored state");
	const auto editSprite = editLevel->GetActorsByType<SpriteActor>()[0].lock();
	Check(!editSprite->FindComponent<vega::script::ScriptComponent>()->HasBegunPlay(), "Edit world must not start script lifecycles");
	auto emptyScene = scene;
	emptyScene["entities"] = Json::array();
	auto emptyLevel = std::make_shared<HeadlessSceneLevel>(nullptr, emptyScene);
	emptyLevel->BeginPlayInternal();
	Check(emptyLevel->Respawn().spawnLocation == sf::Vector2f{ 3000.f, 1500.f }, "No PlayerStart must use the arena default spot");
	World componentWorld{ nullptr };
	const auto componentActor = componentWorld.SpawnActor<SpriteActor>().lock();
	componentActor->AddComponent<SceneEntityComponent>("", "Composition test");
	for (const auto& name : { "HealthComponent", "ShieldComponent", "EnergyComponent", "LightComponent", "CollisionComponent" })
	{
		const auto* info = engineContext.Types().Find(name);
		Check(info && info->addComponent && info->removable, "Suitable components must be addable and removable");
		info->addComponent(*componentActor);
	}
	componentActor->FindComponent<HealthComponent>()->SetMaxHealth(42.f);
	Check(componentActor->RemoveComponent(*componentActor->FindComponent<SpriteComponent>()), "SpriteActor must support sprite removal");
	const auto composition = SceneSerializer::Save(componentWorld);
	World compositionRoundTrip{ nullptr };
	Check(SceneSerializer::Load(compositionRoundTrip, composition).warnings.empty() && SceneSerializer::Save(compositionRoundTrip) == composition,
		"Game addable components and removed sprite must round trip exactly");
	for (const auto& name : { "MovementComponent", "PlayerMovementComponent", "ScriptComponent" })
		Check(!engineContext.Types().Find(name)->addComponent, "Owner-specific movement and scripts must not be generically addable");
	const auto* dummyType = engineContext.Types().Find("DummyEnemy");
	for (const auto& name : { "HealthComponent", "ShieldComponent", "EnergyComponent", "MovementComponent" })
		Check(dummyType->RequiresComponent(name), "Ships must inherit required core components");
	Check(!dummy->RemoveComponent(dummy->GetHealthComponent()), "Ship health removal must be refused");
	Check(!world.GetActorsByTypeIncludingPending<Asteroid>()[0].lock()->RemoveComponent(
		*world.GetActorsByTypeIncludingPending<Asteroid>()[0].lock()->FindComponent<HealthComponent>()), "Asteroid health removal must be refused");
	return failures ? 1 : 0;
}
