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



namespace ly
{
	LevelOne::LevelOne(Application* owningApp)
		:GameLevel(owningApp),
		mPlayerSpaceShip{},
		mChaosStage{}
	{
	
	}

	void LevelOne::OnGameStart()
	{
		LOG("LevelOne::OnGameStart called");
		
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