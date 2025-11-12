#pragma once

#include "framework/World.h"
#include "framework/TimerManager.h"

namespace ly
{
	class Vanguard;
	class PlayerSpaceShip;
	class LevelOne : public World
	{

	public:
		LevelOne(Application* owningApp);

		weak_ptr<PlayerSpaceShip> mPlayerSpaceShip;

	protected:
		virtual void BeginPlay() override;
		virtual void Tick(float deltaTime) override;

	private:
		virtual void InitGameStages() override;
		void PlayerShipDestroyed(Actor* destroyedActor);
		void GameOver();
	};
}