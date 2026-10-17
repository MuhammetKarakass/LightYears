#pragma once
#include "gameplay/GameStage.h"

namespace vega
{
	class WaitStage : public GameStage
	{
	public:
		WaitStage(World* world, float waitDuration = 5.f);

		virtual void BeginStage() override;

	private:
		float mWaitDuration;
	};
}