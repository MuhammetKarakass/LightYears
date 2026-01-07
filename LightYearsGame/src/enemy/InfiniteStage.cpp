#include "enemy/InfiniteStage.h"
#include "enemy/EnemySpaceShip.h"
#include "framework/World.h"
#include "enemy/Vanguard.h"
#include "enemy/TwinBlade.h"
#include "enemy/Hexagon.h"
#include "enemy/UFO.h"
#include "gameConfigs/GameplayConfig.h"
#include "gameplay/HealthComponent.h"
#include "framework/Actor.h"


namespace ly
{
	InfiniteStage::InfiniteStage(World* world) :
		GameStage(world),
		mCurrentWave{ 0 },
		mDifficultyMultiplier{ 1.f },
		mWaveActive{ false },
		mEnemiesSpawned{ 0 },
		mEnemiesAlive{ 0 },
		mCurrentSpawnIndex{ 0 },
		mCurrentWaveEnemyCount{ 0 },
		mCurrentWaveSpawnInterval{ 0.f },
		mCurrentSpawnPattern{},
		mQueueSpawnTimer{ 0.f },
		mUsingPatternQueue{ false },
		mPatternEnemyCount{ 0 },
		mLastIndex{ -1 },
		mNextWaveSpawnInterval{ 3.f },
		mAsteroidSpawnIntervalRange{15.f,20.f},
		mAsteroidSpeedRange{ 200.f,300.f },
		mAsteroidSpawner{ shared_ptr<AsteroidSpawner>(new AsteroidSpawner(world,
			AsteroidSpawnerConfig
			{
				{mAsteroidSpawnIntervalRange.first, mAsteroidSpawnIntervalRange.second},
				{mAsteroidSpeedRange.first, mAsteroidSpeedRange.second},
				{.85f,1.15f},
				{30.f,50.f},
				{40.f,60.f},
				1.f,
				true,
				1
			})) }
	{
		InitializeSpawnGrid();
	}

	void InfiniteStage::BeginStage()
	{
		GameStage::BeginStage();

		onNotification.Broadcast("Infinite Stage Started!", 1.f, 2.f, 1.f, sf::Vector2f{ GetWorld()->GetWindowSize().x / 2.f, GetWorld()->GetWindowSize().y / 2.f }, 24.f, sf::Color::Red);
		TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &InfiniteStage::StartWave, mNextWaveSpawnInterval, false);
	}

	void InfiniteStage::TickStage(float deltaTime)
	{
		GameStage::TickStage(deltaTime);
		if (mWaveActive && mEnemiesAlive <= 0 && mEnemiesSpawned >= mCurrentWaveEnemyCount)
		{
			CompleteWave();
		}
		ProcessSpawnQueue(deltaTime);
	}

	void InfiniteStage::StartWave()
	{
		mCurrentWave++;
		mDifficultyMultiplier = CalculateDifficultyMultiplier();
		mCurrentWaveEnemyCount = CalculateEnemyCount();
		mCurrentWaveSpawnInterval = CalculateSpawnInterval();
		mCurrentSpawnPattern = SelectSpawnPattern();
		mEnemiesSpawned = 0;
		mEnemiesAlive = 0;
		mCurrentSpawnIndex = 0;
		mWaveActive = true;
		
		if (IsSpecialWave())
		{
			GeneratePatternSequence(mCurrentSpawnPattern);
		}
		else
		{
			mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &InfiniteStage::SpawnEnemy, mCurrentWaveSpawnInterval, true);
		}

		if (mCurrentWave == 6)
		{
			mAsteroidSpawner->StartSpawning();
		}
		
		if (mCurrentWave > 5)
		{
			AsteroidDifficulty();
		}
	}

	void InfiniteStage::CompleteWave()
	{
		mWaveActive = false;
		mWaveTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &InfiniteStage::StartWave, mNextWaveSpawnInterval, false);
	}

	weak_ptr<EnemySpaceShip> InfiniteStage::SpawnEnemyAtLocation(int enemyType, const sf::Vector2f& location)
	{
		weak_ptr<EnemySpaceShip> enemy;
		switch (enemyType)
		{
			case 0:
				enemy = GetWorld()->SpawnActor<Vanguard>(GameData::Ship_Enemy_Vanguard);
				break;
			case 1:
				enemy = GetWorld()->SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
				break;
			case 2:
				enemy = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
				break;
			case 3:
				enemy = SpawnUFOWithDynamicPath();
				break;
			default:
				break;
		}
		if (auto e = enemy.lock())
		{
			e->SetActorLocation(location);
			sf::Vector2f velocity = e->GetVelocity();
			float randSpeed = RandRange(0.85f, 1.15f);
			e->SetVelocity(velocity * randSpeed);
			e->onActorDestroyed.BindAction(GetWeakPtr(), &InfiniteStage::OnEnemyDestroyed);
			mEnemiesAlive++;
			mEnemiesSpawned++;
		}
		return enemy;
	}

	void InfiniteStage::SpawnEnemy()
	{
		if (mEnemiesSpawned >= mCurrentWaveEnemyCount)
		{
			TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
			return;
		}

		int enemyType = mCurrentWave < 5 ? RandRange(0, 2) : RandRange(0, 3);
		sf::Vector2f spawnLocation = GetSpawnLocation();
		SpawnEnemyAtLocation(enemyType, spawnLocation);
		mCurrentSpawnIndex++;
	}

	weak_ptr<EnemySpaceShip> InfiniteStage::SpawnUFOWithDynamicPath()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		
		float spawnType = RandRange(0.f, 1.f);
		
		sf::Vector2f spawnPos;
		sf::Vector2f velocity;
		
		if (spawnType < 0.65f)
		{
			float spawnSide = RandRange(0.f, 1.f);
			float spawnLocX = (spawnSide < 0.5f) ? -100.f : windowSize.x + 100.f;
			
			float upperHalfHeight = windowSize.y / 2.f;
			float spawnLocY = RandRange(0.f, upperHalfHeight);
			
			spawnPos = sf::Vector2f{spawnLocX, spawnLocY};
			
			sf::Vector2f center{windowSize.x / 2.f, windowSize.y / 2.f};
			sf::Vector2f direction = center - spawnPos;
			NormalizeVector(direction);
			velocity = direction * 240.f;
		}
		else
		{
			float spawnX = RandRange(100.f, windowSize.x - 100.f);
			spawnPos = sf::Vector2f{spawnX, -50.f};
			
			float horizontalOffset = RandRange(-150.f, 150.f);
			sf::Vector2f target{
				spawnPos.x + horizontalOffset,
				windowSize.y + 50.f
			};
			
			sf::Vector2f direction = target - spawnPos;
			NormalizeVector(direction);
			velocity = direction * 100.f;
		}
		
		weak_ptr<UFO> ufo = GetWorld()->SpawnActor<UFO>(
			GameData::Ship_Enemy_UFO,
			velocity
		);
		
		if (auto u = ufo.lock())
		{
			u->SetActorLocation(spawnPos);
			sf::Vector2f vel = u->GetVelocity();
			float randSpeed = RandRange(0.85f, 1.15f);
			u->SetVelocity(vel * randSpeed);
			
			u->onActorDestroyed.BindAction(GetWeakPtr(), &InfiniteStage::OnEnemyDestroyed);
			
			mEnemiesAlive++;
			mEnemiesSpawned++;
		}
		
		return ufo;
	}

	void InfiniteStage::AsteroidDifficulty()
	{
		mAsteroidSpawnIntervalRange.first = std::max(5.f, mAsteroidSpawnIntervalRange.first - 0.5f);
		mAsteroidSpawnIntervalRange.second = std::max(8.f, mAsteroidSpawnIntervalRange.second - 0.5f);
		
		mAsteroidSpeedRange.first += 5.f;
		mAsteroidSpeedRange.second += 5.f;

		mAsteroidSpawner->SetSpawnTimerRange(mAsteroidSpawnIntervalRange.first, mAsteroidSpawnIntervalRange.second);
		mAsteroidSpawner->SetSpeedRange(mAsteroidSpeedRange.first, mAsteroidSpeedRange.second);
		mNextWaveSpawnInterval = std::max(1.5f, mNextWaveSpawnInterval - 0.1f);
	}

	void InfiniteStage::OnEnemyDestroyed(Actor* actor)
	{
		mEnemiesAlive--;
	}

	int InfiniteStage::CalculateEnemyCount() const
	{
		return std::min(5 + mCurrentWave * 2, 25);
	}

	float InfiniteStage::CalculateSpawnInterval() const
	{
		return std::max(1.5f - mCurrentWave * 0.075f, 0.5f);
	}

	float InfiniteStage::CalculateDifficultyMultiplier() const
	{
		return 1.0f + mCurrentWave / 5.f * 0.25f;
	}

	SpawnPattern InfiniteStage::SelectSpawnPattern() const
	{
		if (mCurrentWave <= 4)
		{
			return SpawnPattern::VFormation;
		}
		else if (mCurrentWave <= 8)
		{
			return SpawnPattern::ReserveVFormation;
		}
		else if (mCurrentWave <= 12)
		{
			return SpawnPattern::Snake;
		}
		else if (mCurrentWave <= 16)
		{
			return SpawnPattern::Scatter;
		}
		else if (mCurrentWave <= 20)
		{
			return SpawnPattern::Zipper;
		}
		return SpawnPattern::Scatter;
	}

	sf::Vector2f InfiniteStage::GetSpawnLocation()
	{
		int gridIndex;

		do
		{
			gridIndex = RandRange(0, int(mSpawnGrid.size() - 1));
		} while (mLastIndex == gridIndex && mEnemiesSpawned > 0);
		mLastIndex = gridIndex;	

		return GetGridPosition(gridIndex);
	}

	void InfiniteStage::InitializeSpawnGrid()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		float spacing = windowSize.x / 10.f;
		for (int i = 0; i < mSpawnGrid.size(); i++)
		{
			mSpawnGrid[i] = sf::Vector2f{ spacing * (i + 1),-100.f };
		}
	}

	sf::Vector2f InfiniteStage::GetGridPosition(int index) const
	{
		if (index < 0 || index >= mSpawnGrid.size())
		{
			return mSpawnGrid[mSpawnGrid.size() / 2];
		}

		sf::Vector2f basePos = mSpawnGrid[index];
		float noiseX = RandRange(-10.f, 10.f);
		float noiseY = RandRange(-10.f, 10.f);
		return basePos += sf::Vector2f{ noiseX, noiseY };
	}

	bool InfiniteStage::IsSpecialWave() const
	{
		return mCurrentWave < 20 ? mCurrentWave % 4 == 0 : mCurrentWave % 3 == 0;
	}

	void InfiniteStage::GeneratePatternSequence(SpawnPattern pattern)
	{
		while(!mSpawnQueue.empty())
			mSpawnQueue.pop();

		mUsingPatternQueue = true;
		mPatternEnemyCount = 0;
		mQueueSpawnTimer = 0.f;

		switch(pattern)
		{
			case SpawnPattern::VFormation:
			{
				int midIndex = ((int)mSpawnGrid.size() - 1) / 2;
				int lhsIndex = midIndex - 1;
				int rhsIndex = midIndex + 1;

				mSpawnQueue.push(SpawnQueueEntry{midIndex, RandRange(0.4f, 0.6f), 0});
				mPatternEnemyCount++;
				
				while (mPatternEnemyCount < 8)
				{
					bool spawned = false;
					if (lhsIndex >= 0) {
						mSpawnQueue.push(SpawnQueueEntry{ lhsIndex, RandRange(0.4f, 0.6f), 0 });
						lhsIndex--;
						mPatternEnemyCount++;
						spawned = true;
					}

					if (rhsIndex < mSpawnGrid.size()) {
						mSpawnQueue.push(SpawnQueueEntry{ rhsIndex, RandRange(0.2f, 0.4f), 0 });
						rhsIndex++;
						mPatternEnemyCount++;
						spawned = true;
					}
					if (!spawned) break;
				}
				break;
			}

			case SpawnPattern::ReserveVFormation:
			{
				int midIdx = ((int)mSpawnGrid.size() - 1) / 2;
				int startIndex = 0;
				int endIndex = (int)mSpawnGrid.size() - 1;

				while (startIndex <= midIdx && endIndex >= midIdx)
				{
					mSpawnQueue.push(SpawnQueueEntry{ startIndex, RandRange(0.4f, 0.6f), 0 });
					mSpawnQueue.push(SpawnQueueEntry{ endIndex, RandRange(0.4f, 0.6f), 0 });
					startIndex++;
					endIndex--;
					mPatternEnemyCount += 2;
				}
				mSpawnQueue.push(SpawnQueueEntry{ midIdx, RandRange(0.4f, 0.6f), 0 });
				mPatternEnemyCount += 1;
				break;
			}

			case SpawnPattern::Snake:
			{
				for (int i = 0; i < mSpawnGrid.size(); ++i)
				{
					mSpawnQueue.push(SpawnQueueEntry{ i, 0.5f, 0 });
					mPatternEnemyCount++;
				}
				break;
			}

			case SpawnPattern::Scatter:
			{ 
				for (int i = 0; i < mSpawnGrid.size(); ++i)
				{
					int randGrid = RandRange(0, (int)mSpawnGrid.size() - 1);
					mSpawnQueue.push(SpawnQueueEntry{ randGrid, RandRange(0.3f, 0.7f), 0 });
					mPatternEnemyCount++;
				}
				break;
			}

			case SpawnPattern::Zipper:
			{ 
				int leftIndex = 0;
				int rightIndex = (int)mSpawnGrid.size() - 1;
				while (leftIndex <= rightIndex)
				{
					mSpawnQueue.push(SpawnQueueEntry{ leftIndex, 0.4f, 0 });
					mPatternEnemyCount++;
					
					if (leftIndex != rightIndex)
					{
						mSpawnQueue.push(SpawnQueueEntry{ rightIndex, 0.3f, 0 });
						mPatternEnemyCount++;
					}

					leftIndex++;
					rightIndex--;
				}
				break;
			}
			
			default:
				mUsingPatternQueue = false;
				break;
		}
	}

	void InfiniteStage::ProcessSpawnQueue(float deltaTime)
	{
		if (!mUsingPatternQueue)
			return;

		mQueueSpawnTimer += deltaTime;
		while(!mSpawnQueue.empty())
		{
			const SpawnQueueEntry& entry = mSpawnQueue.front();

			if (mQueueSpawnTimer >= entry.spawnDelay)
			{
				SpawnEnemyAtLocation(entry.enemyType, GetGridPosition(entry.gridIndex));

				mSpawnQueue.pop();
				mQueueSpawnTimer = 0.f;
			}
			else
				break;
		}
		if (mSpawnQueue.empty())
		{
			mUsingPatternQueue = false;

			if(mEnemiesSpawned< mCurrentWaveEnemyCount)
			{
				mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(), &InfiniteStage::SpawnEnemy, mCurrentWaveSpawnInterval, true);
			}
		}
	}

	void InfiniteStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
		TimerManager::GetGameTimerManager().ClearTimer(mWaveTimerHandle);
	}
}