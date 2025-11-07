#include "gameplay/WaitStage.h"
#include "framework/TimerManager.h"

namespace ly
{
	WaitStage::WaitStage(World* world, float waitDuration):
		GameStage(world),
		mWaitDuration{waitDuration}
	{
	}
	void WaitStage::BeginStage()
	{
		GameStage::BeginStage();
		TimerManager::GetTimerManager().SetTimer(GetWeakPtr(), &WaitStage::FinishStage, mWaitDuration);
	}
}