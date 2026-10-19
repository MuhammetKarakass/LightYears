#include "enemy/VanguardStage.h"
#include "level/authoring/CampaignSetupActor.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include <framework/World.h>
#include "environment/Asteroid.h"

namespace vega
{
	VanguardStage::VanguardStage(World* world) : VanguardStage{ world, VanguardStageSettings{} } {}
	VanguardStage::VanguardStage(World* world, const VanguardStageSettings& settings) :
		GameStage(world),
		mMinSpawnInterval{ settings.minSpawnInterval },
		mMaxSpawnInterval{ settings.maxSpawnInterval },
		mSpawnDistanceToEdge{ settings.spawnDistanceToEdge },
		mSwitchInterval{ settings.switchInterval },
		mRightSpawnLoc{ 0.f,0.f },
		mLeftSpawnLoc{ 0.f,0.f },
		mMidSpawnLoc{ 0.f,0.f },
		mSpawnLoc{ 0.f,0.f },
		mSpawnTimerHandle{},
		mSwitchTimerHandle{},
		mRowsToSpawn{ settings.rows },
		mRowSpawnCount{ 0 },
		mVanguardPerRow{ settings.perRow },
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

		float spawnInterval = RandRange(mMinSpawnInterval, mMaxSpawnInterval);
		mSpawnTimerHandle = GetWorld()->GetTimerManager().SetTimer(GetWeakPtr(),&VanguardStage::SpawnVanguard,spawnInterval,true);
		++mRowSpawnCount;
	}

	void VanguardStage::SpawnVanguard()
	{
		content::SpawnEnemy(
			*GetWorld(),
			EnemyIds::ApproachGunnerBasic,
			{ mSpawnLocIndex[RandomizeSpawnLoc()], mSpawnLoc.y }
		);
		++mCurrentRowVanguardCount;

		if (mVanguardPerRow == mCurrentRowVanguardCount)
		{
			GetWorld()->GetTimerManager().ClearTimer(mSpawnTimerHandle);
			mSwitchTimerHandle = GetWorld()->GetTimerManager().SetTimer(GetWeakPtr(),&VanguardStage::SwitchRow, mSwitchInterval,false);
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
		GetWorld()->GetTimerManager().ClearTimer(mSwitchTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(mSpawnTimerHandle);
	}

}

