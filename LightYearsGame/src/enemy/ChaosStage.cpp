#include "enemy/ChaosStage.h"
#include "enemy/Vanguard.h"
#include "enemy/TwinBlade.h"
#include "enemy/Hexagon.h"
#include "enemy/UFO.h"
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

		weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>(GameData::Ship_Enemy_Vanguard);
		vanguard.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for(unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);

			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();

			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>(GameData::Ship_Enemy_Vanguard);
					vanguard.lock()->SetActorLocation(nextSpawnLoc);
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

		weak_ptr<TwinBlade> twinBlade= GetWorld()->SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
		twinBlade.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for (unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<TwinBlade> twinBlade = GetWorld()->SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
					twinBlade.lock()->SetActorLocation(nextSpawnLoc);
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

		weak_ptr<Hexagon> hexagon= GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
		hexagon.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for (unsigned int i = 2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<Hexagon> hexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
					hexagon.lock()->SetActorLocation(nextSpawnLoc);
				},
				delay,
				false
			);
		}

		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&ChaosStage::SpawnUFO,
			mSpawnInterval
		);
	}

	void ChaosStage::SpawnUFO()
	{
		auto[spawnLoc, velocity] = GetSpawnPropertiesUFO();
		weak_ptr<UFO> ufo=GetWorld()->SpawnActor<UFO>(GameData::Ship_Enemy_UFO,velocity);
		ufo.lock()->SetActorLocation(spawnLoc);

		for(unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			auto [spawnLoc, velocity] = GetSpawnPropertiesUFO();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, spawnLoc, velocity]() {
					weak_ptr<UFO> ufo = GetWorld()->SpawnActor<UFO>(GameData::Ship_Enemy_UFO,velocity);
					ufo.lock()->SetActorLocation(spawnLoc);
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
			int enemyType = RandRange(1, 4);
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
		if (enemyType == 4)
		{
			auto [spawnLoc, velocity] = GetSpawnPropertiesUFO();
			weak_ptr<UFO> ufo = GetWorld()->SpawnActor<UFO>(GameData::Ship_Enemy_UFO,velocity);
			ufo.lock()->SetActorLocation(spawnLoc);
		}

		else 
		{
			if(enemyType==1)
			{
				weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>(GameData::Ship_Enemy_Vanguard);
				vanguard.lock()->SetActorLocation(GetRandomSpawnLocationTop());
			}
			else if(enemyType==2)
			{
				weak_ptr<TwinBlade> twinBlade = GetWorld()->SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
				twinBlade.lock()->SetActorLocation(GetRandomSpawnLocationTop());
			}
			else if(enemyType==3)
			{
				weak_ptr<Hexagon> hexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
				hexagon.lock()->SetActorLocation(GetRandomSpawnLocationTop());
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

	sf::Vector2f ChaosStage::GetRandomSpawnLocationSide()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		float spawnSide = RandRange(0.f, 1.f);
		float spawnLocX = 0.f;

		if(spawnSide < 0.5f)
		{
			spawnLocX = -100.f;
		}
		else
		{
			spawnLocX = windowSize.x + 100.f;
		}

		float halfHeight = windowSize.y/2.f;
		float spawnLocY = RandRange(0.f, halfHeight);

		return sf::Vector2f{ spawnLocX, spawnLocY };
	}
	std::pair<sf::Vector2f, sf::Vector2f> ChaosStage::GetSpawnPropertiesUFO()
	{
		sf::Vector2f spawnLoc = GetRandomSpawnLocationSide();
		auto windowSize = GetWorld()->GetWindowSize();
		auto center = sf::Vector2f{ windowSize.x / 2.f, windowSize.y / 2.f };

		sf::Vector2f directionToCenter = center - spawnLoc;
		sf::Vector2f normalizedVector = NormalizeVector(directionToCenter);

		sf::Vector2f velocity = normalizedVector * 200.f;

		return {spawnLoc, velocity};
	}
}