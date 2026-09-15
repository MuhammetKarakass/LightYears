#include "enemy/HexagonStage.h"
#include <framework/World.h>
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"

namespace ly
{
	HexagonStage::HexagonStage(World* world):
		GameStage{world},
		mSpawnInterval{5.f},
		mSpawnGroupAmt{7},
		mCurrentSpawnCount{0},
		mMidSpawnLoc{world->GetWindowSize().x/2.f, -100.f}
	{

	}
	void HexagonStage::BeginStage()
	{
		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(),&HexagonStage::SpawnHexagon, mSpawnInterval, true);
	}
	void HexagonStage::TickStage(float deltaTime)
	{
	}

	void HexagonStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
	}
	void HexagonStage::SpawnHexagon()
	{

       //TODO: hafif bir randomize ekle
		const auto spawn = [this](const sf::Vector2f& location)
		{
			content::SpawnEnemy(*GetWorld(), EnemyIds::RangeKeeperBasic, location);
		};
		if(mCurrentSpawnCount % 6 == 0)
		{
			spawn(mMidSpawnLoc);
			spawn({ mMidSpawnLoc.x - 150.f, mMidSpawnLoc.y - 150.f });
			spawn({ mMidSpawnLoc.x + 150.f, mMidSpawnLoc.y - 150.f });
			mCurrentSpawnCount += 3;
		}

		else
		{
			spawn({ mMidSpawnLoc.x, mMidSpawnLoc.y - 150.f });
			spawn({ mMidSpawnLoc.x - 150.f, mMidSpawnLoc.y });
			spawn({ mMidSpawnLoc.x + 150.f, mMidSpawnLoc.y });
			mCurrentSpawnCount += 3;
		}

		if (mCurrentSpawnCount >= mSpawnGroupAmt*3)
		{
			FinishStage();
		}
	}
}

