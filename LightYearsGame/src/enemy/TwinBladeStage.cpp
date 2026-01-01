#include "enemy/TwinBladeStage.h"
#include "enemy/TwinBlade.h"
#include <framework/World.h>

namespace ly {
	TwinBladeStage::TwinBladeStage(World* world):
		GameStage(world),
		mSpawnInterval{ {1.f,3.f} },
		mSpawnLocations{},
		mSpawnTimerHandle{},
		mSpawnCount{ 10 },
		mCurrentSpawnCount{ 0 },
		mLastSpawnLoc{0.f, 0.f}
	{
		
	}
	void TwinBladeStage::BeginStage()
	{
		AddSpawnLocations();
		// Ýlk spawn için rastgele interval
		SpawnTwinBlade();
	}
	void TwinBladeStage::TickStage(float deltaTime)
	{
		GameStage::TickStage(deltaTime);
	}
	void TwinBladeStage::SpawnTwinBlade()
	{
		weak_ptr<TwinBlade> newTwinBlade = GetWorld()->SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
		
		newTwinBlade.lock()->SetActorLocation(
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
			}()
		);

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