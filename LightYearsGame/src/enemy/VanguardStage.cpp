#include "enemy/VanguardStage.h"
#include "enemy/Vanguard.h"
#include <framework/World.h>

namespace ly
{
	VanguardStage::VanguardStage(World* world) :
		GameStage(world),
		mMinSpawnInterval{ 0.5f },
		mMaxSpawnInterval{ 2.f },
		mSpawnDistanceToEdge{ 100.f },
		mSwitchInterval{ 5.f },
		mRightSpawnLoc{ 0.f,0.f },
		mLeftSpawnLoc{ 0.f,0.f },
		mMidSpawnLoc{ 0.f,0.f },
		mSpawnLoc{ 0.f,0.f },
		mSpawnTimerHandle{},
		mSwitchTimerHandle{},
		mRowsToSpawn{ 1 },
		mRowSpawnCount{ 0 },
		mVanguardPerRow{ 5 },
		mCurrentRowVanguardCount{ 0 },

		mLastRandNum{ 1 },
		mCurrentRandNum{ 1 },

		mLastSpawnLocIndex{ RandRange(0, 2) },
		mCurrentSpawnLocIndex{ mLastSpawnLocIndex },

		mSpawnLocIndex{}
	{
	}
	void VanguardStage::BeginStage()
	{
		auto windowSize = GetWorld()->GetWindowSize();
		mLeftSpawnLoc = sf::Vector2f{ mSpawnDistanceToEdge,-100.f };
		mRightSpawnLoc = sf::Vector2f{ windowSize.x - mSpawnDistanceToEdge ,-100.f };
		mMidSpawnLoc = sf::Vector2f{ windowSize.x / 2.f, -100.f};

		SwitchRow();
	}

	void VanguardStage::SwitchRow()
	{

		if (mRowsToSpawn == mRowSpawnCount)
		{
			FinishStage();
			return;
		}

		RandomizeRowLoc();


		if (mCurrentRandNum == 1)	
			mSpawnLoc = mRightSpawnLoc;

		else if (mCurrentRandNum == 2)	
			mSpawnLoc = mMidSpawnLoc;

		else  
			mSpawnLoc = mLeftSpawnLoc;

		mSpawnLocIndex[0] = mSpawnLoc.x -60.f;
		mSpawnLocIndex[1] = mSpawnLoc.x;
		mSpawnLocIndex[2] = mSpawnLoc.x +60.f;

		mSpawnTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(),&VanguardStage::SpawnVanguard,mMaxSpawnInterval,true);
		++mRowSpawnCount;
	}

	void VanguardStage::SpawnVanguard()
	{
		weak_ptr<Vanguard> newVanguard = GetWorld()->SpawnActor<Vanguard>();
		//newVanguard.lock()->SetActorLocation(RandomVector(sf::Vector2f{mSpawnLoc.x-100,mSpawnLoc.y}, sf::Vector2f{ mSpawnLoc.x + 100,mSpawnLoc.y }));
		newVanguard.lock()->SetActorLocation(sf::Vector2f {mSpawnLocIndex[RandomizeSpawnLoc()],mSpawnLoc.y });
		++mCurrentRowVanguardCount;

		if (mVanguardPerRow == mCurrentRowVanguardCount)
		{
			TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
			mSwitchTimerHandle = TimerManager::GetGameTimerManager().SetTimer(GetWeakPtr(),&VanguardStage::SwitchRow, mSwitchInterval,false);
			mCurrentRowVanguardCount = 0;
		}
	}

	void VanguardStage::RandomizeRowLoc()
	{
		mLastRandNum = mCurrentRandNum;
		do 
		{
			mCurrentRandNum = RandRange(1, 3);

		} while (mCurrentRandNum == mLastRandNum);
	}

	int VanguardStage::RandomizeSpawnLoc()
	{
		mLastSpawnLocIndex = mCurrentSpawnLocIndex;
		do
		{
			mCurrentSpawnLocIndex = RandRange(0, static_cast<int>(mSpawnLocIndex.size()) - 1);

		} while (mLastSpawnLocIndex == mCurrentSpawnLocIndex);

		return mCurrentSpawnLocIndex;
	}



	void VanguardStage::TickStage(float deltaTime)
	{
		GameStage::TickStage(deltaTime);
	}
	void VanguardStage::StageFinished()
	{
		TimerManager::GetGameTimerManager().ClearTimer(mSwitchTimerHandle);
		TimerManager::GetGameTimerManager().ClearTimer(mSpawnTimerHandle);
	}

}