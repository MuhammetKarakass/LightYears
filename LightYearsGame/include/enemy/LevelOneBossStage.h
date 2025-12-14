#pragma once

#include "gameplay/GameStage.h"
#include <string>

namespace ly

{
	class LevelOneBoss;
	class Actor;
	class LevelOneBossStage : public GameStage
	{
	public:
		LevelOneBossStage(World* world);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

		Delegate<const std::string&, float, float> onBossHealthBarCreated;
		Delegate<> onArrivedLocation;
		Delegate<LevelOneBoss*> onBossSpawned;

		weak_ptr<LevelOneBoss> GetBoss() const { return mBoss; };

	private:
		virtual void StageFinished() override;
		virtual void BossDestroyed(Actor* bossActor);
		void ArrivedLocation();

		bool mBossArrivedLocation = false;

		weak_ptr<LevelOneBoss> mBoss;
	};
}
