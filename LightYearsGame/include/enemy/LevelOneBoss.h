#pragma once

#include "enemy/EnemySpaceShip.h"
#include "weapon/BulletShooter.h"
#include "weapon/ThreeWayShooter.h"
#include "weapon/FrontalWiper.h"
#include "weapon/bulletShooter/MultiShooter.h"
#include "weapon/bulletShooter/ShooterPresets.h"

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
		void Shoot();
		void ShootBaseShooters();
		void ShootThreeWayShooter();
		void ShootFrontalWipers();
		void SetStage(int stage);
		void BossHealthChanged(float amt,float currentHealth, float maxHealth);

		float mSpeed;
		float mBaseSpeed;
		float mSwitchDistanceToEdge;
		int mStage;
		bool mCanShoot;

		unique_ptr<BulletShooter> mBaseShooterLeft;
		unique_ptr<BulletShooter> mBaseShooterRight;
		unique_ptr<ThreeWayShooter> mThreeWayShooter;
		unique_ptr<FrontalWiper> mFrontalWiperLeft;
		unique_ptr<FrontalWiper> mFrontalWiperRight;
		unique_ptr<BulletShooter> mLastStageShooterLeft;
		unique_ptr<BulletShooter> mLastStageShooterRight;
		unique_ptr<MultiShooter> mFan;
		unique_ptr<MultiShooter> mDual;

		ShipDefinition mBossShipDef = ShipDefinition(
			"SpaceShooterRedux/PNG/Enemies/boss.png",
			3000.f,
			{ 100.f,0.f },
			200.f,
			300,
			(int)ly::ExplosionType::Boss,
			{
				EngineMount{ {-80.f,30.f},GameData::Engine_Red_PointLightDef },
				EngineMount{ {80.f,30.f},GameData::Engine_Red_PointLightDef }
			},
			GameData::Laser_Red_BulletDef,
			true,
			{ 0.f,100.f },
			0.5f,
			LevelOneBoss::GetDefaultRewards()
		);
	};
}