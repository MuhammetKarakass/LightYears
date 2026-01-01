#include "enemy/HexagonStage.h"
#include <framework/World.h>
#include "enemy/Hexagon.h"

namespace ly
{
	HexagonStage::HexagonStage(World* world):
		GameStage{world},
		mSpawnInterval{7.5f},
		mSpawnGroupAmt{5},
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

		weak_ptr<Hexagon> newHexagon;
       //TODO: hafif bir randomize ekle
		if(mCurrentSpawnCount % 6 == 0)
		{
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(mMidSpawnLoc);
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(sf::Vector2f{ mMidSpawnLoc.x - 150.f, mMidSpawnLoc.y - 150.f });
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(sf::Vector2f{ mMidSpawnLoc.x + 150.f, mMidSpawnLoc.y - 150.f });
			mCurrentSpawnCount += 3;
		}

		else
		{
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(sf::Vector2f{ mMidSpawnLoc.x , mMidSpawnLoc.y - 150.f });
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(sf::Vector2f{ mMidSpawnLoc.x - 150.f, mMidSpawnLoc.y });
			newHexagon = GetWorld()->SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
			newHexagon.lock()->SetActorLocation(sf::Vector2f{ mMidSpawnLoc.x + 150.f, mMidSpawnLoc.y });
			mCurrentSpawnCount += 3;
		}

		if (mCurrentSpawnCount >= mSpawnGroupAmt*3)
		{
			FinishStage();
		}
	}
}