#include "enemy/UFO.h"
#include "enemy/UFOStage.h"
#include <framework/World.h>

namespace ly
{
	UFOStage::UFOStage(World* world):
		GameStage{world},
		mSpawnInterval{{2.0f,2.5f}},
		mSpawnAmt{30},
		mCurrentSpawnCount{0}
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
		ShipDefinition ufoDef = GameData::Ship_Enemy_UFO;
		sf::Vector2f spawnLoc = RandomSpawnLoc();
		auto windowSize = GetWorld()->GetWindowSize();
		sf::Vector2f center{ windowSize.x / 2.f, windowSize.y / 2.f };
		sf::Vector2f spawnLocToCenter{ center.x - spawnLoc.x, center.y - spawnLoc.y };
		NormalizeVector(spawnLocToCenter);

		float speed = ufoDef.speed.y;
		sf::Vector2f spawnVelocity = spawnLocToCenter * speed;
		weak_ptr<UFO> newUFO = GetWorld()->SpawnActor<UFO>(ufoDef, spawnVelocity);
		newUFO.lock()->SetActorLocation(spawnLoc);
	}
}