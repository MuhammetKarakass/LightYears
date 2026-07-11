#include "enemy/LevelOneBoss.h"
#include "gameplay/HealthComponent.h"
#include "gameConfigs/PointLightConfig.h"
#include "gameConfigs/WeaponConfig.h"

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
			EngineMount{ {0.f,-110.f},LightingData::Engine_Red_PointLightDef },

		},
		LevelOneBoss::GetDefaultRewards(),
		WeaponData::Boss_Base_PrimaryWeaponDef
	);

	LevelOneBoss::LevelOneBoss(World* world)
		: EnemySpaceShip(world,mBossShipDef),
		mSpeed(mBossShipDef.speed.x),
		mSwitchDistanceToEdge(100.f),
		mStage{1},
		mCanShoot{ false },
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
		ly::AbilityDefinition threeWay = AbilityData::MakePrimaryFireAbilityDefinition(WeaponData::Boss_ThreeWay_PrimaryWeaponDef);
		threeWay.slot = AbilitySlot::Ability1;
		threeWay.inputLabel = "";
		GetCombatRuntime().GetAbilities().GrantAbility(threeWay);
		ly::AbilityDefinition frontalSweep = AbilityData::MakePrimaryFireAbilityDefinition(WeaponData::Boss_FrontalSweep_PrimaryWeaponDef);
		frontalSweep.slot = AbilitySlot::Ability2;
		frontalSweep.inputLabel = "";
		GetCombatRuntime().GetAbilities().GrantAbility(frontalSweep);

		ly::AbilityDefinition lastStage = AbilityData::MakePrimaryFireAbilityDefinition(WeaponData::Boss_LastStage_PrimaryWeaponDef);
		lastStage.slot = AbilitySlot::Ability3;
		lastStage.inputLabel = "";
		GetCombatRuntime().GetAbilities().GrantAbility(lastStage);

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
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::PrimaryFire, mCanShoot);
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability1, mCanShoot);
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability2, mCanShoot && mStage >= 3);
		GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability3, mCanShoot && mStage == 4);
	}

	void LevelOneBoss::SetStage(int stage)
	{
		mStage = stage;
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


