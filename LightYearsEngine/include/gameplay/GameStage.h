#pragma once

#include "framework/Object.h"
#include "framework/Core.h"
#include "framework/Delegate.h"

namespace ly
{
	class World;
	class GameStage : public Object
	{
		
	public:
		GameStage(World* world);

		Delegate<> onStageFinished;

		const World* GetWorld() const { return mWorld; }
		World* GetWorld() { return mWorld; }

		virtual void BeginStage();
		virtual void TickStage(float deltaTime);

		bool IsStageFinished() const { return mStageFinished; }
		void FinishStage();


	private:
		World* mWorld;
		bool mStageFinished;

		virtual void StageFinished();
	};
}
