#pragma once

#include <SFML/Graphics.hpp>
#include "framework/TimerManager.h"
#include "gameplay/GameStage.h"
#include <array>


namespace vega
{
	struct VanguardStageSettings;
	class VanguardStage : public GameStage
	{
	public:
		VanguardStage(World* world);
		VanguardStage(World* world, const VanguardStageSettings& settings);
		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:
		float mMinSpawnInterval;
		float mMaxSpawnInterval;

		float mSpawnDistanceToEdge;

		float mSwitchInterval;

		sf::Vector2f mRightSpawnLoc;
		sf::Vector2f mLeftSpawnLoc;
		sf::Vector2f mMidSpawnLoc;
		sf::Vector2f mSpawnLoc;

		TimerHandle mSpawnTimerHandle;
		TimerHandle mSwitchTimerHandle;

		int mRowsToSpawn;
		int mRowSpawnCount;

		int mVanguardPerRow;
		int mCurrentRowVanguardCount;

		int mLastRandNum;
		int mCurrentRandNum;

		int mLastSpawnLocIndex;
		int mCurrentSpawnLocIndex;

		std::array<float, 3> mSpawnLocIndex;

		virtual void StageFinished() override;
		void SpawnVanguard();
		void SwitchRow();
		void RandomizeRowLoc();
		int RandomizeSpawnLoc();
	};
}

