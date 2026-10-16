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
		SceneLevel(Application* app, nlohmann::json scene, SettingsApplier applySettings = {}, bool editMode = false);
		void ApplySettings(const nlohmann::json& settings) { if (mApplySettings) mApplySettings(settings); }
		bool IsEditMode() const override { return mEditMode; }
		bool DispatchEvent(const sf::Event& event) override;
		const SceneLoadResult& GetSceneLoadResult() const { return mLoadResult; }
	protected:
		void BeginPlay() override;
		ArenaDefinition CreateArenaDefinition() const override;
		PlayerRespawnDefinition CreatePlayerRespawnDefinition() const override;
	private:
		nlohmann::json mScene;
		SettingsApplier mApplySettings;
		SceneLoadResult mLoadResult;
		bool mEditMode = false;
	};
}
