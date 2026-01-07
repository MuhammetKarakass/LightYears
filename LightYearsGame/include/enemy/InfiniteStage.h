#pragma once

#include <queue>
#include "gameplay/GameStage.h"
#include "framework/TimerManager.h"
#include "framework/MathUtility.h"
#include "environment/AsteroidSpawner.h"

namespace ly
{
	enum class SpawnPattern
	{
		VFormation,
		Scatter,
		ReserveVFormation,
		Snake,
		Zipper
	};

	struct SpawnQueueEntry
	{
		int gridIndex;
		float spawnDelay;
		int enemyType;
	};

	class Actor;
	class World;
	class EnemySpaceShip;

	class InfiniteStage : public GameStage 
	{
	public:
		InfiniteStage(World* world);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:
		void StartWave();
		void CompleteWave();

		weak_ptr<EnemySpaceShip> SpawnEnemyAtLocation(int enemyType,const sf::Vector2f& location);
		weak_ptr<EnemySpaceShip> SpawnUFOWithDynamicPath();
		void SpawnEnemy();
		void AsteroidDifficulty();
		void OnEnemyDestroyed(Actor* actor);

		int CalculateEnemyCount() const;
		float CalculateSpawnInterval() const;
		float CalculateDifficultyMultiplier() const;
		SpawnPattern SelectSpawnPattern() const;

		sf::Vector2f GetSpawnLocation();

		void InitializeSpawnGrid();
		sf::Vector2f GetGridPosition(int index) const;
		bool IsSpecialWave() const;
		void GeneratePatternSequence(SpawnPattern pattern);
		void ProcessSpawnQueue(float deltaTime);

		virtual void StageFinished() override;

		int mCurrentWave;
		float mDifficultyMultiplier;
		bool mWaveActive;
		int mEnemiesSpawned;
		int mEnemiesAlive;
		int mCurrentSpawnIndex;
		float mNextWaveSpawnInterval;

		int mCurrentWaveEnemyCount;
		float mCurrentWaveSpawnInterval;
		SpawnPattern mCurrentSpawnPattern;

		std::pair <float, float> mAsteroidSpawnIntervalRange;
		std::pair <float, float> mAsteroidSpeedRange;

		TimerHandle mWaveTimerHandle;
		TimerHandle mSpawnTimerHandle;

		std::array<sf::Vector2f, 9> mSpawnGrid;
		std::queue<SpawnQueueEntry> mSpawnQueue;
		float mQueueSpawnTimer;
		bool mUsingPatternQueue;
		int mPatternEnemyCount;
		int mLastIndex;

		shared_ptr<AsteroidSpawner> mAsteroidSpawner;
	};
}