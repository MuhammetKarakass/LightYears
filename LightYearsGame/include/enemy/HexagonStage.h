#pragma once 

#include <gameplay/GameStage.h>
#include <framework/TimerManager.h>
#include <SFML/Graphics.hpp>

namespace vega
{
	struct HexagonStageSettings;
	class HexagonStage: public GameStage
	{
	public:
		HexagonStage(World* world);
		HexagonStage(World* world, const HexagonStageSettings& settings);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:
		virtual void StageFinished() override;
		void SpawnHexagon();

		float mSpawnInterval;
		int mSpawnGroupAmt;
		int mCurrentSpawnCount;

		TimerHandle mSpawnTimerHandle;
		sf::Vector2f mMidSpawnLoc;
		
	};
}



