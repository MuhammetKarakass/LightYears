#pragma once

#include "enemy/EnemySpaceShip.h"
#include "gameplay/ability/AbilitySystem.h"
#include "environment/AsteroidSpawner.h"

namespace ly
{
	class PrimaryWeaponController;

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
		void TickAbilities(float deltaTime);
		void SetStage(int stage);
		void BossHealthChanged(float amt,float currentHealth, float maxHealth);
		PrimaryWeaponController* GetPrimaryWeaponController(AbilitySlot slot);

		float mSpeed;
		float mBaseSpeed;
		float mSwitchDistanceToEdge;
		int mStage;
		bool mCanShoot;
		bool flag = false;

		AbilitySystem mAbilitySystem;

		static const ShipDefinition mBossShipDef;

		shared_ptr<AsteroidSpawner> mAsteroidSpawner;
	};
}
