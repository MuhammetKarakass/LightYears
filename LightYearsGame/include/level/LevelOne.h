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

	private:
		virtual void InitGameStages() override;
		void PlayerShipDestroyed(Actor* destroyedActor);
		void ConnectChaosStageToHUD();
		void ConnectTheBossStageToHUD();
		void OnBossSpawned(LevelOneBoss* boss);

		weak_ptr<ChaosStage> mChaosStage;
		weak_ptr<LevelOneBossStage> mBossStage;
	};
}