#pragma once

//#include "framework/World.h"
#include "level/GameLevel.h"
#include "framework/TimerManager.h"

namespace ly
{
	class Actor;
	class PlayerSpaceShip;
	class ChaosStage;
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

		virtual void OnRestartLevel() override;
		virtual void GameOver() override;

		virtual void OnGamePaused() override;
		virtual void OnGameResumed() override;

	private:
		virtual void InitGameStages() override;
		void PlayerShipDestroyed(Actor* destroyedActor);
		void ConnectChaosStageToHUD();
		void ConnectInfiniteStageToHUD();
		void SpawnCosmetics();

		weak_ptr<ChaosStage> mChaosStage;
		weak_ptr<InfiniteStage> mInfStage;
		weak_ptr<BackGroundActor> mBackgroundActor;
		weak_ptr<BackgroundLayer> mPlanetsLayer;
		weak_ptr<BackgroundLayer> mMeteorsLayer;
	};
}

