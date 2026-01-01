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
#include "enemy/Hexagon.h"
#include "enemy/TwinBlade.h"
#include "enemy/UFO.h"
#include "enemy/Vanguard.h"
#include "gameConfigs/GameplayConfig.h"

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
		GameLevel::OnGameStart();

		SpawnCosmetics();

		Player& newPlayer = PlayerManager::GetPlayerManager().CreateNewPlayer();
		mPlayerSpaceShip=newPlayer.SpawnSpaceShip(this);
		if (!mPlayerSpaceShip.lock()) return;
		mPlayerSpaceShip.lock()->onActorDestroyed.BindAction(GetWeakPtr(), &LevelOne::PlayerShipDestroyed);

		weak_ptr<Vanguard> vanguardEnemy = SpawnActor<Vanguard>(GameData::Ship_Enemy_Vanguard);
		if (auto vanguard = vanguardEnemy.lock())
		{
			vanguard->SetActorLocation(sf::Vector2f{ 100, 100.f });
			vanguard->SetVelocity(sf::Vector2f{ 0.f, 0.f });
		}
		weak_ptr<TwinBlade> twinBladeEnemy = SpawnActor<TwinBlade>(GameData::Ship_Enemy_TwinBlade);
		if (auto twinBlade = twinBladeEnemy.lock())
		{
			twinBlade->SetActorLocation(sf::Vector2f{ 200.f, 100.f });
			twinBlade->SetVelocity(sf::Vector2f{ 0.f, 0.f });
		}
		weak_ptr<Hexagon> hexagonEnemy = SpawnActor<Hexagon>(GameData::Ship_Enemy_Hexagon);
		if (auto hexagon = hexagonEnemy.lock())
		{
			hexagon->SetActorLocation(sf::Vector2f{ 300.f, 100.f });
			hexagon->SetVelocity(sf::Vector2f{ 0.f, 0.f });
		}
		weak_ptr<UFO> ufoEnemy = SpawnActor<UFO>(GameData::Ship_Enemy_UFO,sf::Vector2f{ 0.f, 0.f });
		if (auto ufo = ufoEnemy.lock())
		{
			ufo->SetActorLocation(sf::Vector2f{ 400.f, 100.f });
			ufo->SetVelocity(sf::Vector2f{ 0.f, 0.f });
		}
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
		// Base class'ı çağır (müzik kontrolü için)
		GameLevel::OnGamePaused();

		// Background layer'ları pause et
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
		// Base class'ı çağır (müzik kontrolü için)
		GameLevel::OnGameResumed();

		// Background layer'ları resume et
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
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});

		AddGameStage(shared_ptr<VanguardStage>{new VanguardStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<HexagonStage>{new HexagonStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<TwinBladeStage>{new TwinBladeStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 10.f)});
		AddGameStage(shared_ptr<UFOStage>{new UFOStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 15.f)});

		shared_ptr<ChaosStage> chaosStage = shared_ptr<ChaosStage>{ new ChaosStage(this) };
		mChaosStage = chaosStage;
		AddGameStage(chaosStage);
		chaosStage->onStageStarted.BindAction(GetWeakPtr(), &LevelOne::ConnectChaosStageToHUD);

		shared_ptr<LevelOneBossStage> bossStage = shared_ptr<LevelOneBossStage>{ new LevelOneBossStage(this) };
		mBossStage = bossStage;
		AddGameStage(bossStage);
		bossStage->onStageStarted.BindAction(GetWeakPtr(), &LevelOne::ConnectTheBossStageToHUD);


		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 15.f)});
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
				bossStage->onNotification.BindAction(
					hud->GetWeakPtr(),
					&GameHUD::ShowDynamicNotification
				);

				bossStage->onBossHealthBarCreated.BindAction(
					hud->GetWeakPtr(),
					&GameHUD::CreateBossHealthBar
				);

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

		// ---- PLANETS (with optional light) ----
		
			// Gezegen ışığı preset'i
			PointLightDefinition planetLightDef(
				"SpaceShooterRedux/Shaders/point_light.frag",   // shaderPath
				sf::Color{ 180, 200, 255, 255 },               // color (soğuk beyaz-mavi)
				1.15f,                                         // intensity
				sf::Vector2f{ 200.f, 200.f },                  // size (base, scale ile büyüyecek)
				false,                                         // shouldStretch
				false,                                         // complexTrail
				0.15f,                                         // taperAmount
				0.9f,                                          // edgeSoftness
				1.0f                                           // shapeRoundness
			);

			// Işıklı/ışıksız karışık gezegen listesi
			List<BackgroundLayerDefinition> planetDefs =
			{
				GameData::Environment::Meteor1,
				GameData::Environment::Meteor2,
				GameData::Environment::Planet_Blue,
				GameData::Environment::Planet_Earth_Blue,
				GameData::Environment::Planet_Green,
				GameData::Environment::Star_Orange,
				GameData::Environment::Planet_Orange
			};

			mPlanetsLayer = SpawnActor<BackgroundLayer>(planetDefs);
			mPlanetsLayer.lock()->SetRandomVisibility(true);

			mPlanetsLayer.lock()->SetUseDepthColor(true);
			mPlanetsLayer.lock()->SetSizeRange(0.65f, 1.f);
			mPlanetsLayer.lock()->SetVelocityRange(sf::Vector2f{ 0.f,30.f }, sf::Vector2f{ 0.f,80.f });
		
			mPlanetsLayer.lock()->SetSpriteCount(2);

		// ---- METEORS (no light) ----
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