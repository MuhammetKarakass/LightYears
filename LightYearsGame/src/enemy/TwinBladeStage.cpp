#include "enemy/TwinBladeStage.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include <framework/MathUtility.h>
#include <framework/World.h>

namespace ly {
	TwinBladeStage::TwinBladeStage(World* world):
		GameStage(world),
		mSpawnInterval{ {.75f,1.5f} },
		mSpawnLocations{},
		mSpawnTimerHandle{},
		mSpawnCount{ 30 },
		mCurrentSpawnCount{ 0 },
		mLastSpawnLoc{0.f, 0.f}
	{
		
	}
	void TwinBladeStage::BeginStage()
	{
		AddSpawnLocations();
		SpawnTwinBlade();
	}
	void TwinBladeStage::TickStage(float deltaTime)
	{
		GameStage::TickStage(deltaTime);
	}
	void TwinBladeStage::SpawnTwinBlade()
	{
		sf::Vector2f spawnLocation =
			[this]() ->sf::Vector2f 
			{
				if (mSpawnLocations.size() == 1)
				{
					return mSpawnLocations[0];
				}
				sf::Vector2f newLocation;
				do
				{
					int randomIndex = RandRange(0, static_cast<int>(mSpawnLocations.size()) - 1);
					newLocation = mSpawnLocations[randomIndex];
				} while (newLocation == mLastSpawnLoc);
				mLastSpawnLoc = newLocation;
				return newLocation;
			}();
		content::SpawnEnemy(*GetWorld(), EnemyIds::StrafeSkirmisherBasic, spawnLocation);

		++mCurrentSpawnCount;

		if (mCurrentSpawnCount >= mSpawnCount)
		{
			FinishStage();
			return;
		}
		else
		{
			
			float nextSpawnInterval = RandRange(mSpawnInterval[0], mSpawnInterval[1]);
			
			mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(
				GetWeakPtr(), 
				&TwinBladeStage::SpawnTwinBlade, 
				nextSpawnInterval, 
				false 
			);
		}
	}
	void TwinBladeStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
	}

	void TwinBladeStage::AddSpawnLocations()
	{
		auto windowSize = GetWorld()->GetWindowSize();

		for(int i=1 ; i<=5; ++i)
		{
			mSpawnLocations.push_back(sf::Vector2f{ i * 100.f, -100.f });
		}
	}
}

