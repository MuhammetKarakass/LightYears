#pragma once

#include "framework/TimerManager.h"

namespace ly
{
	class World;
	class Actor;

	struct AsteroidSpawnerConfig
	{
		std::pair<float, float> spawnIntervalRange;
		std::pair<float, float> speedRange;
		std::pair<float, float> sizeRange;
		std::pair<float, float> healthRange;
		std::pair<float, float> damageRange;
		float toTargetRatio;
		bool withTimer;
		int spawnAmt;
	};

	class AsteroidSpawner : public Object
	{
	public:
		AsteroidSpawner(World* world, const AsteroidSpawnerConfig& config);
		
		void StartSpawning();
		void StopSpawning();
		bool IsSpawning() const {return mIsSpawning; };

		void SetConfig(const AsteroidSpawnerConfig& config);
		void SetSpawnTimerRange(float min,float max);
		void SetSpeedRange(float min, float max);
		void SetAsteroidCount(int count);

		void SpawnAsteroidBatch(int amount);
		void SpawnSingleAsteroid();

	private:
		void SpawnAsteroid();
		void SpawnAsteroidInternal();

		World* mWorld;
		AsteroidSpawnerConfig mConfig;
		TimerHandle mSpawnTimer;
		bool mIsSpawning;
		weak_ptr<Actor> mTargetActor;
	};
}