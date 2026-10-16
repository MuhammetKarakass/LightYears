#include "level/SceneLevel.h"
#include "level/ArenaSandboxSetup.h"
#include "level/PlayerStart.h"
#include "reflection/GameReflection.h"
#include "player/PlayerManager.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/debug/Log.h>

namespace ly
{
	SceneLevel::SceneLevel(Application* app, nlohmann::json scene, SettingsApplier applySettings, bool editMode)
		: ArenaLevel{ app }, mScene(std::move(scene)), mApplySettings{ std::move(applySettings) }, mEditMode{ editMode }
	{
		SetPaused(editMode);
	}

	bool SceneLevel::DispatchEvent(const sf::Event& event)
	{
		return mEditMode || ArenaLevel::DispatchEvent(event);
	}

	void SceneLevel::BeginPlay()
	{
		RegisterGameTypes();
		mLoadResult = SceneSerializer::Load(*this, mScene);
		if (mScene.is_object() && mScene.contains("settings") && mScene["settings"].is_object())
		{
			const auto& settings = mScene["settings"];
			try
			{
				if (mApplySettings) mApplySettings(settings);
				if (settings.contains("playerShip"))
				{
					const auto ship = settings["playerShip"].get<std::string>();
					if (ship.rfind("Ship.Player.", 0) == 0 && content::ShipContentCatalog::FindById(ship))
					{
						if (!mEditMode) PlayerManager::GetPlayerManager().SetSelectedShipId(ship);
					}
					else mLoadResult.warnings.push_back("Unknown playerShip setting: " + ship);
				}
			}
			catch (const std::exception& error) { mLoadResult.warnings.push_back(std::string{ "Scene settings: " } + error.what()); }
		}
		if (!mEditMode && (!mScene.contains("settings") || !mScene["settings"].contains("playerShip")))
			PlayerManager::GetPlayerManager().SetSelectedShipId(DefaultPlayerShipId);
		if (!mEditMode) for (const auto& marker : GetActorsByTypeIncludingPending<PlayerStart>())
		{
			if (auto start = marker.lock())
			{
				PlayerManager::GetPlayerManager().SetSelectedShipId(start->GetShip());
				break;
			}
		}
		for (const auto& warning : mLoadResult.warnings) LY_GAME_WARN("Scene: %s", warning.c_str());
		// ArenaLevel owns the exact same player, respawn, HUD and follow-camera
		// path as ArenaTestLevel. Entities must exist before its system setup.
		if (!mEditMode) ArenaLevel::BeginPlay();
		LY_GAME_INFO("SceneLevel: scene-loaded %d entities (%s)", mLoadResult.spawned, mEditMode ? "edit mode" : "play mode");
	}

	ArenaDefinition SceneLevel::CreateArenaDefinition() const
	{
		return CreateSandboxArenaDefinition();
	}

	PlayerRespawnDefinition SceneLevel::CreatePlayerRespawnDefinition() const
	{
		auto definition = ConfigureSandboxPlayerRespawn(ArenaLevel::CreatePlayerRespawnDefinition());
		for (const auto& marker : GetActorsByTypeIncludingPending<PlayerStart>())
		{
			if (auto start = marker.lock())
			{
				definition.spawnLocation = start->GetActorLocation();
				definition.playerShipId = start->GetShip();
				break;
			}
		}
		return definition;
	}
}
