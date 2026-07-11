#pragma once

#include "enemy/EnemySpaceShip.h"
#include "environment/AsteroidSpawner.h"

namespace ly
{
	class LevelOneBoss : public EnemySpaceShip
	{
	public:
		LevelOneBoss(World* world);

		virtual void Tick(float deltaTime) override;
		virtual void BeginPlay() override;

		virtual void ApplyDamage(float amt) override;
		void BossArrivedLocation();
	protected:
		static List<WeightedReward> GetDefaultRewards();

	private:
		void CheckMove();
		void UpdateWeaponFireIntent();
		void SetStage(int stage);
		void BossHealthChanged(float amt,float currentHealth, float maxHealth);

		float mSpeed;
		float mBaseSpeed;
		float mSwitchDistanceToEdge;
		int mStage;
		bool mCanShoot;
		bool flag = false;

		static const ShipDefinition mBossShipDef;

		shared_ptr<AsteroidSpawner> mAsteroidSpawner;
	};
}


