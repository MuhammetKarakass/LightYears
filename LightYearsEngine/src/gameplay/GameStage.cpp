#include "gameplay/GameStage.h"

namespace ly
{
	GameStage::GameStage(World* world):
		mWorld{world},
		mStageFinished{false}
	{
	}

	void GameStage::BeginStage()
	{
	}

	void GameStage::TickStage(float deltaTime)
	{
	}

	void GameStage::FinishStage()
	{
		mStageFinished = true;
		StageFinished();
		onStageFinished.Broadcast();
	}

	void GameStage::StageFinished()
	{
		
	}
}