#pragma once

#include <SFML/Graphics.hpp>
#include <framework/TimerManager.h>
#include <gameplay/GameStage.h>
#include <utility> 

namespace ly
{
	class UFOStage : public GameStage
	{
	public:
		UFOStage(World* world);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:
		virtual void StageFinished() override;
		
		sf::Vector2f RandomSpawnLoc();
		void SpawnUFO();
		void SpawnUFOPair();
		

		List<float> mSpawnInterval;
		int mSpawnAmt;
		int mCurrentSpawnCount;

		TimerHandle mSpawnTimerHandlePair;
		TimerHandle mSpawnTimerHandleSecond;
	};
}