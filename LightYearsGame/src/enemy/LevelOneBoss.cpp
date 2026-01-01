#include "enemy/LevelOneBoss.h"
#include "gameplay/HealthComponent.h"

namespace ly
{
	LevelOneBoss::LevelOneBoss(World* world)
		: EnemySpaceShip(world,mBossShipDef),
		mSpeed(mBossShipDef.speed.y),
		mSwitchDistanceToEdge(100.f),
		mStage{1},
		mCanShoot{ false },
		mBaseShooterLeft{ std::make_unique<BulletShooter>(this,GameData::Laser_Red_BulletDef,0.5f,sf::Vector2f{-50.f,50.f}) },
		mBaseShooterRight{ std::make_unique<BulletShooter>(this,GameData::Laser_Red_BulletDef,0.5f,sf::Vector2f{50.f,50.f}) },
		mThreeWayShooter{ std::make_unique<ThreeWayShooter>(this,GameData::Laser_Red_BulletDef,2.f,sf::Vector2f{0.f,100.f}) },
		mFrontalWiperLeft{ std::make_unique<FrontalWiper>(this,GameData::Laser_Red_BulletDef,3.f,sf::Vector2f{-100.f,80.f}) },
		mFrontalWiperRight{ std::make_unique<FrontalWiper>(this,GameData::Laser_Red_BulletDef,3.f,sf::Vector2f{100.f,80.f}) },
		mLastStageShooterLeft{ std::make_unique<BulletShooter>(this,GameData::Laser_Red_BulletDef,0.5f,sf::Vector2f{-150.f,50.f}) },
		mLastStageShooterRight{ std::make_unique<BulletShooter>(this,GameData::Laser_Red_BulletDef,0.5f,sf::Vector2f{150.f,50.f}) },
		mDual{
		std::make_unique<MultiShooter>(
			this,
			std::pair<float, float>{1.f,1.f},
			ShooterPresets::Dual(
				GameData::Laser_Red_BulletDef,
				sf::Vector2f{0.f,0.f},
				0.f,
				30.f
			),
			FireMode::Simultaneous,
			0.1f
)
		},

		mFan{ std::make_unique<MultiShooter>(
			this,
			std::pair<float, float>{3.f,6.f},
			ShooterPresets::Fan(
				GameData::Laser_Red_BulletDef,
				sf::Vector2f{0.f,0.f},
				5.f,
				30.f
			),
			FireMode::Alternating,
			0.1f
		) }
	{
		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Boss);
		SetScoreAmt(1000);
		SetCollisionDamage(200.f);
		
	}
	void LevelOneBoss::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		CheckMove();
		Shoot();
	}
	void LevelOneBoss::BeginPlay()
	{
		EnemySpaceShip::BeginPlay();
		HealthComponent& healthComp = GetHealthComponent();
		healthComp.SetInitialHealth(3000.f, 3000.f);
		healthComp.onHealthChanged.BindAction(GetWeakPtr(), &LevelOneBoss::BossHealthChanged);
	}
	void LevelOneBoss::ApplyDamage(float amt)
	{
		if(IsInvulnerable())
		{
			return;
		}
		EnemySpaceShip::ApplyDamage(amt);
	}
	void LevelOneBoss::BossArrivedLocation()
	{
		SetVelocity({ mSpeed,0.f });
		mCanShoot = true;
	}
	void LevelOneBoss::CheckMove()
	{
		if(GetActorLocation().x>GetWindowSize().x - mSwitchDistanceToEdge)
		{
			SetVelocity({ -mSpeed,0.f });
		}
		else if(GetActorLocation().x < mSwitchDistanceToEdge)
		{
			SetVelocity({ mSpeed,0.f });
		}
	}

	void LevelOneBoss::Shoot()
	{
		if(!mCanShoot)
			return;

		ShootBaseShooters();
		ShootThreeWayShooter();
		if(mStage>=3)
		{
			ShootFrontalWipers();
		}
		if(mStage==4)
		{
			if (mLastStageShooterLeft)
			{
				mLastStageShooterLeft->Shoot();
			}
			if (mLastStageShooterRight)
			{
				mLastStageShooterRight->Shoot();
			}
		}
	}

	void LevelOneBoss::ShootBaseShooters()
	{

		if (mBaseShooterLeft)
		{
			mBaseShooterLeft->Shoot();
		}
		if (mBaseShooterRight)
		{
			mBaseShooterRight->Shoot();
		}
		mFan->Shoot();
	}

	void LevelOneBoss::ShootThreeWayShooter()
	{
		if (mThreeWayShooter)
		{
			mThreeWayShooter->Shoot();
		}
	}	

	void LevelOneBoss::ShootFrontalWipers()
	{
		if (mFrontalWiperLeft)
		{
			mFrontalWiperLeft->Shoot();
		}
		if (mFrontalWiperRight)
		{
			mFrontalWiperRight->Shoot();
		}
	}

	void LevelOneBoss::SetStage(int stage)
	{
		mStage = stage;
		mBaseShooterLeft.get()->SetCurrentLevel(stage);
		mBaseShooterRight.get()->SetCurrentLevel(stage);
		mThreeWayShooter.get()->SetCurrentLevel(stage);
		mFrontalWiperLeft.get()->SetCurrentLevel(stage);
		mFrontalWiperRight.get()->SetCurrentLevel(stage);
	}

	void LevelOneBoss::BossHealthChanged(float amt, float currentHealth, float maxHealth)
	{
		float healthPercent = currentHealth / maxHealth;

		if(healthPercent <0.75f && healthPercent >=0.5f)
		{
			SetStage(2);
		}
		else if (healthPercent < 0.5f && healthPercent >= 0.25f)
		{
			SetStage(3);
		}
		else if (healthPercent < 0.25f)
		{
			SetStage(4);
		}
	}

	List<WeightedReward> LevelOneBoss::GetDefaultRewards()
	{
		return {};
	}
}
