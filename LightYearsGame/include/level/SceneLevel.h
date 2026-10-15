#pragma once

#include "level/ArenaLevel.h"
#include <framework/scene/Scene.h>

namespace ly
{
	class SceneLevel : public ArenaLevel
	{
	public:
		// Presentation is application-owned; a callback applies game settings
		// without making the headless game library depend on the visual pipeline.
		using SettingsApplier = std::function<void(const nlohmann::json&)>;
		SceneLevel(Application* app, nlohmann::json scene, SettingsApplier applySettings = {});
		const SceneLoadResult& GetSceneLoadResult() const { return mLoadResult; }
	protected:
		void BeginPlay() override;
		ArenaDefinition CreateArenaDefinition() const override;
		PlayerRespawnDefinition CreatePlayerRespawnDefinition() const override;
	private:
		nlohmann::json mScene;
		SettingsApplier mApplySettings;
		SceneLoadResult mLoadResult;
	};
}
