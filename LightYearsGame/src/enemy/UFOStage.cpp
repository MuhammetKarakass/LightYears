#include "enemy/UFO.h"
#include "enemy/UFOStage.h"
#include <framework/World.h>

namespace ly
{
	UFOStage::UFOStage(World* world):
		GameStage{world},
		mSpawnInterval{{2.5f,3.5f}},
		mSpawnAmt{16},
		mCurrentSpawnCount{0},
		mUFOSpeed{200.f}
	{
	}

	void UFOStage::BeginStage()
	{
		SpawnUFOPair();
	}
	
	void UFOStage::TickStage(float deltaTime)
	{
		GameStage::TickStage(deltaTime);
	}

	void UFOStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandlePair);
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandleSecond);
	}

	sf::Vector2f UFOStage::RandomSpawnLoc()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		float spawnSide = RandRange(0.f, 1.f);
		float spawnLocX = 0.f;

		if (spawnSide < 0.5f)
		{
			spawnLocX = -100.f;
		}
		else
		{
			spawnLocX = windowSize.x + 100.f;
		}

		float upperHalfHeight = windowSize.y / 2.f;
		float spawnLocY = RandRange(0.f, upperHalfHeight);

		return sf::Vector2f{ spawnLocX, spawnLocY };
	}
	
	void UFOStage::SpawnUFOPair()
	{
		SpawnUFO();
		
		mSpawnTimerHandleSecond = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&UFOStage::SpawnUFO,
			1.f,
			false
		);

		mCurrentSpawnCount += 2;

		if (mCurrentSpawnCount >= mSpawnAmt)
		{
			FinishStage();
			return;
		}

		mSpawnTimerHandlePair = TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&UFOStage::SpawnUFOPair,
			RandRange(mSpawnInterval[0], mSpawnInterval[1]),
			false
		);
	}

	void UFOStage::SpawnUFO()
	{
		auto [spawnLoc, spawnVelocity] = GetSpawnProperties();
		
		weak_ptr<UFO> newUFO = GetWorld()->SpawnActor<UFO>(spawnVelocity);
		newUFO.lock()->SetActorLocation(spawnLoc);
	}

	std::pair<sf::Vector2f, sf::Vector2f> UFOStage::GetSpawnProperties()
	{
		sf::Vector2f spawnLoc = RandomSpawnLoc();

		auto windowSize = GetWorld()->GetWindowSize();
		sf::Vector2f center{ windowSize.x / 2.f, windowSize.y / 2.f };
		sf::Vector2f spawnLocToCenter{ center.x - spawnLoc.x, center.y - spawnLoc.y };
		NormalizeVector(spawnLocToCenter);

		sf::Vector2f spawnVelocity = spawnLocToCenter * mUFOSpeed;

		return { spawnLoc, spawnVelocity };
	}
}