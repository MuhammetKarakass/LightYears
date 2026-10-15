#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
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
	class HeadlessSceneLevel : public ly::SceneLevel
	{
	public:
		using SceneLevel::SceneLevel;
		ly::PlayerRespawnDefinition Respawn() const { return CreatePlayerRespawnDefinition(); }
	protected:
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};
}

int main()
{
	using namespace ly;
	using Json = nlohmann::json;
	const auto root = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	AssetManager::GetAssetManager().SetAssetRootDirectory(root.generic_string() + "/");
	vega::script::SetScriptRoot(root);
	vega::script::RegisterScriptTypes();
	RegisterGameTypes();
	Check(GameContentBootstrap::Register(), "Game content bootstrap failed");
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
	const auto* shipProperty = reflect::TypeRegistry::Get().Find("DummyEnemy")->FindProperty("Ship");
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
	return failures ? 1 : 0;
}
