#pragma once

//#include "framework/World.h"
#include "level/GameLevel.h"
#include "framework/TimerManager.h"

namespace ly
{
	class Actor;
	class Vanguard;
	class PlayerSpaceShip;
	class ChaosStage;
	class LevelOneBossStage;
	class LevelOneBoss;
	class BackGroundActor;
	class BackgroundLayer;
	class InfiniteStage;
	class LevelOne : public GameLevel
	{

	public:
		LevelOne(Application* owningApp);

		weak_ptr<PlayerSpaceShip> mPlayerSpaceShip;

	protected:
		virtual void OnGameStart() override;
		virtual void Tick(float deltaTime) override;
		virtual void OnActorSpawned(Actor* actor) override;

		virtual void OnRestartLevel() override;
		virtual void GameOver() override;

		virtual void OnGamePaused() override;
		virtual void OnGameResumed() override;

	private:
		virtual void InitGameStages() override;
		void PlayerShipDestroyed(Actor* destroyedActor);
		void ConnectChaosStageToHUD();
		void ConnectTheBossStageToHUD();
		void ConnectInfiniteStageToHUD();
		void OnBossSpawned(LevelOneBoss* boss);
		void SpawnCosmetics();

		void OnBossStageStarted();
		void OnBossDefeated(Actor* actor);

		weak_ptr<ChaosStage> mChaosStage;
		weak_ptr<LevelOneBossStage> mBossStage;
		weak_ptr<InfiniteStage> mInfStage;
		weak_ptr<BackGroundActor> mBackgroundActor;
		weak_ptr<BackgroundLayer> mPlanetsLayer;
		weak_ptr<BackgroundLayer> mMeteorsLayer;
	};
}