#pragma once
#include "gameplay/GameStage.h"

namespace ly
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