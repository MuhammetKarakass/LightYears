#pragma once

#include "gameplay/GameStage.h"
#include <SFML/Graphics.hpp>
#include "framework/TimerManager.h"
#include "framework/Delegate.h"

namespace ly
{
	class ChaosStage : public GameStage
	{
	public:
		ChaosStage(World* world);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

		Delegate<float> onChaosTimerUpdated;
		Delegate<> onTotalChaosStarted;
		Delegate<> onTotalChaosEnded;



	private:

		virtual void StageFinished() override;
		void SpawnVanguard();
		void SpawnTwinBlade();
		void SpawnHexagon();
		void SpawnUFO();
		void SpawnAsteroid();
		void TotalChaos();
		void SpawnEnemyByType(int enemyType);

		void IncreaseDifficulty();
		void StageDurationFinished();

		sf::Vector2f GetRandomSpawnLocationTop();
		sf::Vector2f GetRandomSpawnLocationSide();

		std::pair<sf::Vector2f, sf::Vector2f> GetSpawnPropertiesUFO();

		float mSpawnInterval;

		float mSpawnIntervalDecrement;

		float mChaosTimer;


		float mSpawnMinDistanceTop;
		List<float> mReservedTopSpawnLocs;

		unsigned int mDifficultyLevel;
		unsigned int mSpawnAmt;

		bool mIsTotalChaosActive{ false };
		bool mTotalChaosTimerActive{ false };

		TimerHandle mSpawnTimerHandle;
		TimerHandle mTotalChaosTimerHandle;  // Total Chaos'un kendini çaðýrmasý için
	};
}