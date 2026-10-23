#pragma once

//#include "framework/World.h"
#include "level/GameLevel.h"
#include "framework/TimerManager.h"
#include "level/LevelLayout.h"

namespace vega
{
	class Actor;
	class PlayerSpaceShip;
	class ChaosStage;
	class BackGroundActor;
	class BackgroundLayer;
	class LevelOne : public GameLevel
	{

	public:
		LevelOne(Application* owningApp);
		using SettingsApplier = std::function<void(const nlohmann::json&)>;
		LevelOne(Application* owningApp, nlohmann::json scene, SettingsApplier applySettings = {}, bool editMode = false);
		bool IsEditMode() const override { return mEditMode; }
		const LevelLayout& GetLayout() const { return mLayout; }
		const SceneLoadResult& GetLoadResult() const { return mLoadResult; }

		weak_ptr<PlayerSpaceShip> mPlayerSpaceShip;

	protected:
		void BeginPlay() override;
		bool DispatchEvent(const sf::Event& event) override;
		virtual void OnGameStart() override;
		virtual void Tick(float deltaTime) override;

		virtual void OnRestartLevel() override;
		virtual void GameOver() override;

		virtual void OnGamePaused() override;
		virtual void OnGameResumed() override;

	private:
		LevelLayout mLayout;
		SettingsApplier mApplySettings;
		SceneLoadResult mLoadResult;
		bool mEditMode = false;
		bool mRestarting = false;
		virtual void InitGameStages() override;
		void PlayerShipDestroyed(Actor* destroyedActor);
		void ConnectChaosStageToHUD();
		void SpawnCosmetics();

		weak_ptr<ChaosStage> mChaosStage;
		weak_ptr<BackGroundActor> mBackgroundActor;
		weak_ptr<BackgroundLayer> mPlanetsLayer;
		weak_ptr<BackgroundLayer> mMeteorsLayer;
	};
}

