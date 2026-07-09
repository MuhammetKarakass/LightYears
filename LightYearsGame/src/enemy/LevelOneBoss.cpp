#include "enemy/LevelOneBoss.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ability/controllers/PrimaryWeaponController.h"

namespace ly
{
	const ShipDefinition LevelOneBoss::mBossShipDef(
		"SpaceShooterRedux/PNG/Enemies/boss.png",
		4000.f,
		{ 100.f,0.f },
		200.f,
		300,
		(int)ly::ExplosionType::Boss,
		{
			EngineMount{ {0.f,-110.f},GameData::Engine_Red_PointLightDef },

		},
		LevelOneBoss::GetDefaultRewards(),
		GameData::Boss_Base_PrimaryWeaponDef
	);

	LevelOneBoss::LevelOneBoss(World* world)
		: EnemySpaceShip(world,mBossShipDef),
		mSpeed(mBossShipDef.speed.x),
		mSwitchDistanceToEdge(100.f),
		mStage{1},
		mCanShoot{ false },
		mAbilitySystem{ this },
		mAsteroidSpawner{ shared_ptr<AsteroidSpawner>(new AsteroidSpawner(world,
			AsteroidSpawnerConfig
			{
				{7.5f,15.f},
				{250.f,350.f},
				{.85f,1.15f},
				{30.f,50.f},
				{40.f,60.f},
				.75f,
				true,
				1
			})) }
	{
		mAbilitySystem.AddController(
			AbilitySlot::PrimaryFire,
			std::make_unique<PrimaryWeaponController>(this, GameData::Boss_Base_PrimaryWeaponDef)
		);
		mAbilitySystem.AddController(
			AbilitySlot::Skill1,
			std::make_unique<PrimaryWeaponController>(this, GameData::Boss_ThreeWay_PrimaryWeaponDef)
		);
		mAbilitySystem.AddController(
			AbilitySlot::Skill2,
			std::make_unique<PrimaryWeaponController>(this, GameData::Boss_FrontalSweep_PrimaryWeaponDef)
		);
		mAbilitySystem.AddController(
			AbilitySlot::Skill3,
			std::make_unique<PrimaryWeaponController>(this, GameData::Boss_LastStage_PrimaryWeaponDef)
		);

		SetActorRotation(180.f);
		SetExplosionType(ExplosionType::Boss);
		SetScoreAmt(1000);
		SetCollisionDamage(200.f);
		for (const auto& mount : mBossShipDef.engineMounts)
		{
			AddLight(GameTags::Ship::Engine_Main, mount.pointLightDef, mount.offset);
		}
		
	}
	void LevelOneBoss::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		CheckMove();
		UpdateWeaponFireIntent();
		TickAbilities(deltaTime);
	}
	void LevelOneBoss::BeginPlay()
	{
		EnemySpaceShip::BeginPlay();
		HealthComponent& healthComp = GetHealthComponent();
		healthComp.SetInitialHealth(mBossShipDef.health,mBossShipDef.health);
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

	void LevelOneBoss::UpdateWeaponFireIntent()
	{
		mAbilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, mCanShoot);
		mAbilitySystem.SetSlotInput(AbilitySlot::Skill1, mCanShoot);
		mAbilitySystem.SetSlotInput(AbilitySlot::Skill2, mCanShoot && mStage >= 3);
		mAbilitySystem.SetSlotInput(AbilitySlot::Skill3, mCanShoot && mStage == 4);
	}

	void LevelOneBoss::TickAbilities(float deltaTime)
	{
		mAbilitySystem.Tick(deltaTime);
	}

	void LevelOneBoss::SetStage(int stage)
	{
		mStage = stage;

		if (PrimaryWeaponController* baseWeapon = GetPrimaryWeaponController(AbilitySlot::PrimaryFire))
		{
			baseWeapon->GetAttributes().shotsPerSecond.currentValue = 2.f + static_cast<float>(stage) * 0.35f;
		}

		if (PrimaryWeaponController* threeWayWeapon = GetPrimaryWeaponController(AbilitySlot::Skill1))
		{
			threeWayWeapon->GetAttributes().shotsPerSecond.currentValue = 0.5f + static_cast<float>(stage) * 0.1f;
		}

		if (PrimaryWeaponController* frontalPatternWeapon = GetPrimaryWeaponController(AbilitySlot::Skill2))
		{
			frontalPatternWeapon->GetAttributes().shotsPerSecond.currentValue = 0.33f + static_cast<float>(stage) * 0.08f;
		}

		if (PrimaryWeaponController* lastStageWeapon = GetPrimaryWeaponController(AbilitySlot::Skill3))
		{
			lastStageWeapon->GetAttributes().shotsPerSecond.currentValue = 2.f + static_cast<float>(stage) * 0.25f;
		}
	}

	PrimaryWeaponController* LevelOneBoss::GetPrimaryWeaponController(AbilitySlot slot)
	{
		return dynamic_cast<PrimaryWeaponController*>(mAbilitySystem.GetController(slot));
	}

	void LevelOneBoss::BossHealthChanged(float amt, float currentHealth, float maxHealth)
	{
		float healthPercent = currentHealth / maxHealth;

		if(healthPercent <0.75f && healthPercent >=0.5f)
		{
			if(flag==false){
				SetStage(2);
				mAsteroidSpawner->StartSpawning();
				mSpeed = mSpeed * 1.5f;
				flag = true;
			}
		}
		else if (healthPercent < 0.5f && healthPercent >= 0.25f)
		{
			if(flag)
			{
				SetStage(3);
				mSpeed = mSpeed * 1.5f;
				mAsteroidSpawner->SetSpawnTimerRange(2.5f, 7.5f);
				flag = false;
			}

		}
		else if (healthPercent < 0.25f)
		{
			if(flag==false){
				SetStage(4);
				mSpeed = mSpeed * 1.5f;
				flag = true;
			}
		}
	}

	List<WeightedReward> LevelOneBoss::GetDefaultRewards()
	{
		return {};
	}
}
