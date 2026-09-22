#include "level/LevelOne.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include <gameplay/GameStage.h>
#include "enemy/VanguardStage.h"
#include "enemy/TwinBladeStage.h"
#include "enemy/HexagonStage.h"
#include <gameplay/WaitStage.h>
#include "player/PlayerManager.h"
#include "widget/GameHUD.h"
#include "framework/Application.h"
#include "enemy/ChaosStage.h"
#include "framework/BackGroundActor.h"
#include "framework/BackgroundLayer.h"
#include "gameConfigs/world/EnvironmentConfig.h"

namespace ly
{
	LevelOne::LevelOne(Application* owningApp)
		:GameLevel(owningApp),
		mPlayerSpaceShip{},
		mChaosStage{},
		mBackgroundActor{},
		mPlanetsLayer{},
		mMeteorsLayer{}
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
		GameLevel::OnGamePaused();

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
		GameLevel::OnGameResumed();

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
		AddGameStage(shared_ptr<TwinBladeStage>{new TwinBladeStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<HexagonStage>{new HexagonStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});

		shared_ptr<ChaosStage> chaosStage = shared_ptr<ChaosStage>{ new ChaosStage(this) };
		mChaosStage = chaosStage;
		AddGameStage(chaosStage);
		chaosStage->onStageStarted.BindAction(GetWeakPtr(), &LevelOne::ConnectChaosStageToHUD);

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

	void LevelOne::SpawnCosmetics()
	{
		mBackgroundActor = SpawnActor<BackGroundActor>("SpaceShooterRedux/Backgrounds/darkPurple.png");

		
			PointLightDefinition planetLightDef(
				"SpaceShooterRedux/Shaders/point_light.frag",
				sf::Color{ 180, 200, 255, 255 },
				1.15f,
				sf::Vector2f{ 200.f, 200.f },
				false,                                       
				false,
				0.15f,                                       
				0.9f,
				1.0f			);

			List<BackgroundLayerDefinition> planetDefs =
			{
				EnvironmentData::Meteor1,
				EnvironmentData::Meteor2,
				EnvironmentData::Planet_Blue,
				EnvironmentData::Planet_Earth_Blue,
				EnvironmentData::Planet_Green,
				EnvironmentData::Star_Orange,
				EnvironmentData::Planet_Orange
			};

			mPlanetsLayer = SpawnActor<BackgroundLayer>(planetDefs);
			mPlanetsLayer.lock()->SetRandomVisibility(true);

			mPlanetsLayer.lock()->SetUseDepthColor(true);
			mPlanetsLayer.lock()->SetSizeRange(0.65f, 1.f);
			mPlanetsLayer.lock()->SetVelocityRange(sf::Vector2f{ 0.f,30.f }, sf::Vector2f{ 0.f,50.f });
		
			mPlanetsLayer.lock()->SetSpriteCount(2);

		mMeteorsLayer = SpawnActor<BackgroundLayer>();
		mMeteorsLayer.lock()->SetPaths({
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny1.png",
			"SpaceShooterRedux/PNG/Meteors/meteorGrey_tiny2.png",
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

	void LevelOne::OnRestartLevel()
	{
		GameLevel::OnRestartLevel();
		GetApplication()->LoadWorld<LevelOne>();
	}
}

