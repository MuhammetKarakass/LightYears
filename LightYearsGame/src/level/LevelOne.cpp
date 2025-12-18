#include "level/LevelOne.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include "enemy/Vanguard.h"
#include <gameplay/GameStage.h>
#include "enemy/VanguardStage.h"
#include "enemy/TwinBladeStage.h"
#include "enemy/HexagonStage.h"
#include "enemy/UFOStage.h"
#include <gameplay/WaitStage.h>
#include "player/PlayerManager.h"
#include "widget/GameHUD.h"
#include "enemy/EnemySpaceShip.h"
#include "framework/Application.h"
#include "enemy/ChaosStage.h"
#include "enemy/LevelOneBossStage.h"
#include "enemy/LevelOneBoss.h"
#include "framework/BackGroundActor.h"
#include "framework/BackgroundLayer.h"



namespace ly
{
	LevelOne::LevelOne(Application* owningApp)
		:GameLevel(owningApp),
		mPlayerSpaceShip{},
		mChaosStage{},
		mBackgroundActor{},
		mPlanetsLayer{},
		mMeteorsLayer{},
		mBossStage{}
	{
	
	}

	void LevelOne::OnGameStart()
	{
		SpawnCosmetics();

		Player& newPlayer = PlayerManager::GetPlayerManager().CreateNewPlayer();
		mPlayerSpaceShip=newPlayer.SpawnSpaceShip(this);
		if (!mPlayerSpaceShip.lock()) return;
		mPlayerSpaceShip.lock()->onActorDestroyed.BindAction(GetWeakPtr(), &LevelOne::PlayerShipDestroyed);
	}

	void LevelOne::PlayerShipDestroyed(Actor* destroyedActor)
	{
		mPlayerSpaceShip = PlayerManager::GetPlayerManager().GetPlayer()->SpawnSpaceShip(this);
		if(!mPlayerSpaceShip.expired())
		{
			mPlayerSpaceShip.lock()->onActorDestroyed.BindAction(GetWeakPtr(), &LevelOne::PlayerShipDestroyed);
		}
		else 
		{
			TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), &LevelOne::GameOver, 3.f, false);
		}
	}

	void LevelOne::GameOver()
	{
		GameLevel::GameOver();
	}

	void LevelOne::OnGamePaused()
	{
		if (auto bgActor = mBackgroundActor.lock())
		{
			bgActor->SetPaused(true);
		}
		if (auto bgLayer = mPlanetsLayer.lock())
		{
			bgLayer->SetPaused(true);
		}

		if(auto meteorsLayer = mMeteorsLayer.lock())
		{
			meteorsLayer->SetPaused(true);
		}
	}

	void LevelOne::OnGameResumed()
	{
		if (auto bgActor = mBackgroundActor.lock())
		{
			bgActor->SetPaused(false);
		}
		if (auto bgLayer = mPlanetsLayer.lock())
		{
			bgLayer->SetPaused(false);
		}

		if(auto meteorsLayer = mMeteorsLayer.lock())
		{
			meteorsLayer->SetPaused(false);
		}
	}

	void LevelOne::InitGameStages()
	{

		shared_ptr<LevelOneBossStage> bossStage = shared_ptr<LevelOneBossStage>{ new LevelOneBossStage(this) };
		mBossStage = bossStage;
		AddGameStage(bossStage);
		bossStage->onStageStarted.BindAction(GetWeakPtr(), &LevelOne::ConnectTheBossStageToHUD);


		shared_ptr<ChaosStage> chaosStage = shared_ptr<ChaosStage>{ new ChaosStage(this) };
		mChaosStage = chaosStage;
		AddGameStage(chaosStage);
		chaosStage->onStageStarted.BindAction(GetWeakPtr(), &LevelOne::ConnectChaosStageToHUD);

		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<UFOStage>{new UFOStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<VanguardStage>{new VanguardStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this,5.f)});
		AddGameStage(shared_ptr<TwinBladeStage>{new TwinBladeStage(this)});
		AddGameStage(shared_ptr<HexagonStage>{new HexagonStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});		
	}

	void LevelOne::ConnectChaosStageToHUD()
	{
		
		if (auto chaosStage = mChaosStage.lock())
		{			
			if (auto hud = GetGameHUD().lock())
			{
				chaosStage->onNotification.BindAction(hud->GetWeakPtr(), &GameHUD::ShowDynamicNotification);
				chaosStage->onTotalChaosStarted.BindAction(hud->GetWeakPtr(), &GameHUD::ShowTimer);
				chaosStage->onChaosTimerUpdated.BindAction(hud->GetWeakPtr(), &GameHUD::UpdateTimer);
				chaosStage->onTotalChaosEnded.BindAction(hud->GetWeakPtr(), &GameHUD::TimerFinished);
			}
		}
	}

	void LevelOne::ConnectTheBossStageToHUD()
	{
		if (auto bossStage = mBossStage.lock())
		{
			if (auto hud = GetGameHUD().lock())
			{
				// Stage-level delegates
				bossStage->onNotification.BindAction(
					hud->GetWeakPtr(),
					&GameHUD::ShowDynamicNotification
				);

				bossStage->onBossHealthBarCreated.BindAction(
					hud->GetWeakPtr(),
					&GameHUD::CreateBossHealthBar
				);

				// ✅ Boss spawn olduğunda bağlantıları yap
				bossStage->onBossSpawned.BindAction(GetWeakPtr(), &LevelOne::OnBossSpawned);
			}
		}
	}

	void LevelOne::OnBossSpawned(LevelOneBoss* boss)
	{
		if (!boss)
		{
			return;
		}

		auto hud = GetGameHUD().lock();
		if (!hud)
		{
			return;
		}
		boss->GetHealthComponent().onHealthChanged.BindAction(
			hud->GetWeakPtr(),
			&GameHUD::BossHealthUpdated
		);

		if (auto bossStage = mBossStage.lock())
		{
			bossStage->onArrivedLocation.BindAction(
				boss->GetWeakPtr(),
				&LevelOneBoss::BossArrivedLocation
			);
		}

		boss->onActorDestroyed.BindAction(
			hud->GetWeakPtr(),
			&GameHUD::RemoveBossHealthBar
		);
	}

	void LevelOne::SpawnCosmetics()
	{
		mBackgroundActor = SpawnActor<BackGroundActor>("SpaceShooterRedux/Backgrounds/darkPurple.png");
		mPlanetsLayer = SpawnActor<BackgroundLayer>();
		mPlanetsLayer.lock()->SetRandomVisibility(true);
		mPlanetsLayer.lock()->SetPaths({
			"SpaceShooterRedux/PNG/Planets/Planet1.png",
			"SpaceShooterRedux/PNG/Planets/Planet2.png",
			"SpaceShooterRedux/PNG/Planets/Planet3.png",
			"SpaceShooterRedux/PNG/Planets/Planet4.png",
			"SpaceShooterRedux/PNG/Planets/Planet5.png",
			"SpaceShooterRedux/PNG/Planets/Planet6.png",
			"SpaceShooterRedux/PNG/Planets/Planet7.png",
			});

		mPlanetsLayer.lock()->SetSpriteCount(2);
		mPlanetsLayer.lock()->SetUseDepthColor(true);
		mPlanetsLayer.lock()->SetSizeRange(0.65f, 1.f);
		mPlanetsLayer.lock()->SetVelocityRange(sf::Vector2f{ 0.f,30.f }, sf::Vector2f{ 0.f,80.f });

		mMeteorsLayer = SpawnActor<BackgroundLayer>();
		mMeteorsLayer.lock()->SetPaths({
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_big1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_big2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_big3.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_big4.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_med1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_med3.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_small1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_small2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_tiny1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_tiny2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_big1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_big2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_big3.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_big4.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_med1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_med2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_small1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_small2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_small1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_small2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_small1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_small2.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_tiny1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorBrown_tiny2.png"

			});

		mMeteorsLayer.lock()->SetSpriteCount(40);
		mMeteorsLayer.lock()->SetUseDepthColor(true);
		mMeteorsLayer.lock()->SetRandomVisibility(false);
		mMeteorsLayer.lock()->SetSizeRange(0.5f, 0.7f);
		mMeteorsLayer.lock()->SetVelocityRange(sf::Vector2f{ 0.f,50.f }, sf::Vector2f{ 0.f,100.f });
	}


	void LevelOne::Tick(float deltaTime)
	{
		World::Tick(deltaTime);
	}

	void LevelOne::OnActorSpawned(Actor* actor)
	{
		if(!actor)
			return;
		if (EnemySpaceShip* enemy = dynamic_cast<EnemySpaceShip*>(actor))
		{

			if (Player* player = PlayerManager::GetPlayerManager().GetPlayer())
			{
				enemy->onScoreAwarded.BindAction(
					player,
					&Player::OnScoreAwarded
				);
			}
			else
			{

			}
		}
	}

	void LevelOne::OnRestartLevel()
	{
		GameLevel::OnRestartLevel();
		GetApplication()->LoadWorld<LevelOne>();
	}

}