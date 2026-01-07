#include "framework/World.h"
#include "environment/AsteroidSpawner.h"
#include "environment/Asteroid.h"


namespace ly
{
	AsteroidSpawner::AsteroidSpawner(World* world, const AsteroidSpawnerConfig& config):
		mWorld{ world },
		mConfig{ config },
		mIsSpawning{ false }
	{
	}

	void AsteroidSpawner::StartSpawning()
	{
		if(mIsSpawning)
			return;
		mIsSpawning = true;
		SpawnAsteroidInternal();
		LOG("Asteroid Spawner Started");
	}
	
	void AsteroidSpawner::StopSpawning()
	{
		if(!mIsSpawning)
			return;
		mIsSpawning = false;
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimer);
	}
	
	void AsteroidSpawner::SetConfig(const AsteroidSpawnerConfig& config)
	{
		mConfig = config;
	}

	void AsteroidSpawner::SetSpawnTimerRange(float min, float max)
	{
		mConfig.spawnIntervalRange = { min, max };
	}

	void AsteroidSpawner::SetSpeedRange(float min, float max)
	{
		mConfig.speedRange = { min, max };
	}

	void AsteroidSpawner::SetAsteroidCount(int count)
	{
		mConfig.spawnAmt = count;
	}

	void AsteroidSpawner::SpawnAsteroidBatch(int amount)
	{
		int spawnAmt = std::max(1, amount);
		
		for(int i = 0; i < spawnAmt; i++)
		{
			float delay = RandRange(0.0f, 0.3f * i);
			TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &AsteroidSpawner::SpawnAsteroid, delay, false);
		}
	}

	void AsteroidSpawner::SpawnSingleAsteroid()
	{
		SpawnAsteroid();
	}

	void AsteroidSpawner::SpawnAsteroid()
	{

			float speed = RandRange(mConfig.speedRange.first, mConfig.speedRange.second);
			float size = RandRange(mConfig.sizeRange.first, mConfig.sizeRange.second);
			float damage = RandRange(mConfig.damageRange.first, mConfig.damageRange.second);
			float health = RandRange(mConfig.healthRange.first, mConfig.healthRange.second);
			float ratio = std::min(mConfig.toTargetRatio, 1.f);

			auto windowSize = mWorld->GetWindowSize();
			sf::Vector2 spawnPosition{ RandRange(0.0f, (float)windowSize.x), -100.0f };

			sf::Vector2f velocity{ 0.0f, 1.f };

			if (mTargetActor.expired() && RandRange(0.0f, 1.0f) < ratio)
			{
				mTargetActor = mWorld->GetActorByLayer(CollisionLayer::Player);
			}

			if (auto targetActor = mTargetActor.lock())
			{
				sf::Vector2f targetPosition = targetActor->GetActorLocation();
				float randomXOffset = RandRange(-50.0f, 50.0f);
				targetPosition.x += randomXOffset;
				sf::Vector2f direction = targetPosition - spawnPosition;
				NormalizeVector(direction);

				velocity = direction;
			}
			else
			{
				velocity = sf::Vector2f{ RandRange(-0.3f, 0.3f), 1.f };
				NormalizeVector(velocity);
			}

			velocity *= speed;

			weak_ptr<Asteroid> asteroid = mWorld->SpawnActor<Asteroid>(velocity, health, damage);

			if (auto newAsteroid = asteroid.lock())
			{
				newAsteroid->SetActorLocation(spawnPosition);
				newAsteroid->SetAsteroidScale(size);
			}
	}

	void AsteroidSpawner::SpawnAsteroidInternal()
	{
		if(!mIsSpawning)
			return;

		int spawnAmt = std::max(1, mConfig.spawnAmt);
		
		for(int i = 0; i < spawnAmt; i++)
		{
			float delay = RandRange(0.0f, 0.3f * i);
			TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &AsteroidSpawner::SpawnAsteroid, delay, false);
		}

		if(mConfig.withTimer && mIsSpawning)
		{
			float nextSpawnIn = RandRange(mConfig.spawnIntervalRange.first, mConfig.spawnIntervalRange.second);
			mSpawnTimer = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), [this]() {
				SpawnAsteroidInternal();
			}, nextSpawnIn, false);
		}
	}
}