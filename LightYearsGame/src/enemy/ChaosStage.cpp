#include "enemy/ChaosStage.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include <framework/MathUtility.h>
#include <framework/World.h>
#include "environment/AsteroidSpawner.h"

namespace ly
{
	ChaosStage::ChaosStage(World* world) :
		GameStage{ world },
		mSpawnInterval{ 3.f },
		mSpawnIntervalDecrement{ 0.05f },
		mChaosTimer{ 60.f},
		mDifficultyLevel{ 1 },
		mSpawnAmt{1},
		mSpawnMinDistanceTop{75.f},
		mMaxAsteroidSpawnCount{ 1 },
		mAsteroidSpawningActive{false},
		mAsteroidSpawner{std::make_shared<AsteroidSpawner>(world,
			AsteroidSpawnerConfig
			{
				{7.5f,15.f},
				{150.f,250.f},
				{.8f,1.f},
				{20.f,50.f},
				{5.f,15.f},
				0.5f,
				true,
				mMaxAsteroidSpawnCount }) },
		mReservedTopSpawnLocs{}
	{
	}
	void ChaosStage::BeginStage()
	{
		GameStage::BeginStage();
		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&ChaosStage::SpawnVanguard,
			mSpawnInterval
		);
	}

	void ChaosStage::TickStage(float deltaTime)
	{
		if (mIsTotalChaosActive && mTotalChaosTimerActive)
		{
			mChaosTimer -= deltaTime;
			onChaosTimerUpdated.Broadcast(mChaosTimer);
			
			if (mChaosTimer <= 0.f && !IsStageFinished())
			{
				mIsTotalChaosActive = false;				
				onTotalChaosEnded.Broadcast();
				
				TimerManager::GetGameTimerManager().SetTimer(
					GetWeakPtr(),
					&ChaosStage::StageDurationFinished,
					0.2f,
					false
				);
			}
		}
	}

	void ChaosStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
		TimerManager::GetGameTimerManager().ClearTimer(mTotalChaosTimerHandle);
	}

	void ChaosStage::SpawnVanguard()
	{
		mReservedTopSpawnLocs.clear();

		content::SpawnEnemy(*GetWorld(), EnemyIds::ApproachGunnerBasic, GetRandomSpawnLocationTop());

		for(unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);

			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();

			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					content::SpawnEnemy(*GetWorld(), EnemyIds::ApproachGunnerBasic, nextSpawnLoc);
				},
				delay,
				false
			);

		}
		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&ChaosStage::SpawnTwinBlade,
			mSpawnInterval
		);
	}

	void ChaosStage::SpawnTwinBlade()
	{
		mReservedTopSpawnLocs.clear();

		content::SpawnEnemy(*GetWorld(), EnemyIds::StrafeSkirmisherBasic, GetRandomSpawnLocationTop());

		for (unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					content::SpawnEnemy(*GetWorld(), EnemyIds::StrafeSkirmisherBasic, nextSpawnLoc);
				},
				delay,
				false
			);
		}

		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&ChaosStage::SpawnHexagon,
			mSpawnInterval
		);
	}

	void ChaosStage::SpawnHexagon()
	{
		mReservedTopSpawnLocs.clear();

		content::SpawnEnemy(*GetWorld(), EnemyIds::RangeKeeperBasic, GetRandomSpawnLocationTop());

		for (unsigned int i = 2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					content::SpawnEnemy(*GetWorld(), EnemyIds::RangeKeeperBasic, nextSpawnLoc);
				},
				delay,
				false
			);
		}

		IncreaseDifficulty();
	}

	void ChaosStage::TotalChaos()
	{
		if (!mTotalChaosTimerActive)
		{
			onTotalChaosStarted.Broadcast(.5f, mChaosTimer, .5f);
			mAsteroidSpawner->SetAsteroidCount(RandRange(2, mMaxAsteroidSpawnCount));
		}
		mTotalChaosTimerActive = true;
		if (IsStageFinished() || !mIsTotalChaosActive)
		{
			return;
		}
		mReservedTopSpawnLocs.clear();

		int batchSize = RandRange(1, 2);

		for (int i = 0; i < batchSize; ++i)
		{
			int enemyType = RandRange(1, 3);
			float microDelay = RandRange(0.4f, 0.8f);

			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, enemyType]() {
					SpawnEnemyByType(enemyType);
				},
				microDelay,
				false
			);
		}
		float nextBatchDelay = RandRange(1.f, 1.3f);
		mTotalChaosTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&ChaosStage::TotalChaos,
			nextBatchDelay,
			false
		);
	}

	void ChaosStage::SpawnEnemyByType(int enemyType)
	{
		if (enemyType >= 1 && enemyType <= 3)
		{
			if(enemyType==1)
			{
				content::SpawnEnemy(*GetWorld(), EnemyIds::ApproachGunnerBasic, GetRandomSpawnLocationTop());
			}
			else if(enemyType==2)
			{
				content::SpawnEnemy(*GetWorld(), EnemyIds::StrafeSkirmisherBasic, GetRandomSpawnLocationTop());
			}
			else if(enemyType==3)
			{
				content::SpawnEnemy(*GetWorld(), EnemyIds::RangeKeeperBasic, GetRandomSpawnLocationTop());
			}
		}
	}

	void ChaosStage::IncreaseDifficulty()
	{
		++mDifficultyLevel;
		
		if(mDifficultyLevel % 3 == 0)
		{
			++mSpawnAmt;
			if (mAsteroidSpawningActive)
			{
				++mMaxAsteroidSpawnCount;
				mAsteroidSpawner->SetAsteroidCount(RandRange(1, mMaxAsteroidSpawnCount));
			}
			if (!mAsteroidSpawningActive)
			{
				mAsteroidSpawner->StartSpawning();
				mAsteroidSpawningActive = true;
			}
		}
		
		mSpawnInterval -= mSpawnIntervalDecrement;

		if (mDifficultyLevel == 12 && !mIsTotalChaosActive)
		{
			mIsTotalChaosActive = true;
			onNotification.Broadcast(std::string{ "SURVIVE!" },
				.5f, 1.5f, .5f,
				sf::Vector2f{ GetWorld()->GetWindowSize().x/2.f, GetWorld()->GetWindowSize().y / 2.f },
				50.f, sf::Color::Red);
			
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				&ChaosStage::TotalChaos,
				10.f,
				false
			);
		}
		else 
		{
			mSpawnTimerHandle= TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				&ChaosStage::SpawnVanguard,
				mSpawnInterval
			);
		}

	}

	void ChaosStage::StageDurationFinished()
	{
		FinishStage();
	}


	sf::Vector2f ChaosStage::GetRandomSpawnLocationTop()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		float edgePadding = 35.f;
		float candidateX = 0.f;
		bool isSafe = false;
		int attempts = 0;

		do
		{
			candidateX = RandRange(edgePadding, static_cast<float>(windowSize.x) - edgePadding);

			isSafe = true;
			for (float reservedX : mReservedTopSpawnLocs)
			{
				if (std::abs(candidateX - reservedX) < mSpawnMinDistanceTop)
				{
					isSafe = false;
					break;
				}
			}
			attempts++;
		} while (!isSafe && attempts < 30);

		mReservedTopSpawnLocs.push_back(candidateX);

		return sf::Vector2f{ candidateX, -100.f };
	}

}

