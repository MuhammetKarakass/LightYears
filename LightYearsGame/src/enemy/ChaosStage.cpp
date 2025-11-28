#include "enemy/ChaosStage.h"
#include "enemy/Vanguard.h"
#include "enemy/TwinBlade.h"
#include "enemy/Hexagon.h"
#include "enemy/UFO.h"
#include <framework/World.h>

namespace ly
{
	ChaosStage::ChaosStage(World* world) :
		GameStage{ world },
		mSpawnInterval{ 3.5f },
		mSpawnIntervalDecrement{ 0.125f },
		mChaosTimer{ 60.f},
		mDifficultyLevel{ 1 },
		mSpawnAmt{1},
		mSpawnMinDistanceTop{75.f},
		mReservedTopSpawnLocs{}
	{
	}
	void ChaosStage::BeginStage()
	{
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
				LOG("*** CHAOS TIMER FINISHED - PREPARING TO END STAGE ***");
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
		LOG("ChaosStage::StageFinished - Clearing all timers");
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
		TimerManager::GetGameTimerManager().ClearTimer(mTotalChaosTimerHandle);
	}

	void ChaosStage::SpawnVanguard()
	{
		mReservedTopSpawnLocs.clear();

		weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>();
		vanguard.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for(unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);

			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();

			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>();
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

		weak_ptr<TwinBlade> twinBlade= GetWorld()->SpawnActor<TwinBlade>();
		twinBlade.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for (unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<TwinBlade> twinBlade = GetWorld()->SpawnActor<TwinBlade>();
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

		weak_ptr<Hexagon> hexagon= GetWorld()->SpawnActor<Hexagon>();
		hexagon.lock()->SetActorLocation(GetRandomSpawnLocationTop());

		for (unsigned int i = 2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			sf::Vector2f nextSpawnLoc = GetRandomSpawnLocationTop();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, nextSpawnLoc]() {
					weak_ptr<Hexagon> hexagon = GetWorld()->SpawnActor<Hexagon>();
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
		weak_ptr<UFO> ufo=GetWorld()->SpawnActor<UFO>(velocity);
		ufo.lock()->SetActorLocation(spawnLoc);

		for(unsigned int i=2; i <= mSpawnAmt; ++i)
		{
			float delay = RandRange(0.1f, 0.3f);
			auto [spawnLoc, velocity] = GetSpawnPropertiesUFO();
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, spawnLoc, velocity]() {
					weak_ptr<UFO> ufo = GetWorld()->SpawnActor<UFO>(velocity);
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
		mTotalChaosTimerActive = true;
		if (IsStageFinished() || !mIsTotalChaosActive)
		{
			LOG("TotalChaos stopped - stage finished or chaos inactive");
			return;
		}

		mReservedTopSpawnLocs.clear();

		int batchSize = RandRange(1, 2);

		for (int i = 0; i < batchSize; ++i)
		{
			int enemyType = RandRange(1, 4); // 1: Vanguard, 2: TwinBlade, 3: Hexagon, 4: UFO
			float microDelay = RandRange(0.1f, 0.4f);

			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				[this, enemyType]() {
					SpawnEnemyByType(enemyType);
				},
				microDelay,
				false
			);
		}
		float nextBatchDelay = RandRange(1.f, 1.75f);
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
			weak_ptr<UFO> ufo = GetWorld()->SpawnActor<UFO>(velocity);
			ufo.lock()->SetActorLocation(spawnLoc);
		}

		else 
		{
			if(enemyType==1)
			{
				weak_ptr<Vanguard> vanguard = GetWorld()->SpawnActor<Vanguard>();
				vanguard.lock()->SetActorLocation(GetRandomSpawnLocationTop());
			}
			else if(enemyType==2)
			{
				weak_ptr<TwinBlade> twinBlade = GetWorld()->SpawnActor<TwinBlade>();
				twinBlade.lock()->SetActorLocation(GetRandomSpawnLocationTop());
			}
			else if(enemyType==3)
			{
				weak_ptr<Hexagon> hexagon = GetWorld()->SpawnActor<Hexagon>();
				hexagon.lock()->SetActorLocation(GetRandomSpawnLocationTop());
			}
		}
	}

	void ChaosStage::IncreaseDifficulty()
	{
		++mDifficultyLevel;
		LOG("ChaosStage difficulty increased to: %d", mDifficultyLevel);
		
		if(mDifficultyLevel % 3 == 0)
		{
			++mSpawnAmt;
		}
		
		mSpawnInterval -= mSpawnIntervalDecrement;

		if (mDifficultyLevel == 2 && !mIsTotalChaosActive)
		{
			LOG("*** TOTAL CHAOS ACTIVATING ***");
			mIsTotalChaosActive = true;
			onTotalChaosStarted.Broadcast();
			LOG("onTotalChaosStarted broadcast sent");
			
			TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(),
				&ChaosStage::TotalChaos,
				15.f,
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

			// 2. ADIM: Sadece mesafe kontrolü yap
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