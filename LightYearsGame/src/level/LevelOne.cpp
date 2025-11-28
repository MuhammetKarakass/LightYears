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
			GameOver();
		}
	}

	void LevelOne::GameOver()
	{
		LOG("GAME OVER*****************************************");

		//TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), &LevelOne::OnRestartLevel, 3.f, false);
	}

	void LevelOne::InitGameStages()
	{
		LOG("LevelOne::InitGameStages called");
		
		// ChaosStage'i oluþtur ve referansýný tut
		shared_ptr<ChaosStage> chaosStage = shared_ptr<ChaosStage>{new ChaosStage(this)};
		mChaosStage = chaosStage;
		AddGameStage(chaosStage);
		
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<UFOStage>{new UFOStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<VanguardStage>{new VanguardStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this,5.f)});
		AddGameStage(shared_ptr<TwinBladeStage>{new TwinBladeStage(this)});
		AddGameStage(shared_ptr<HexagonStage>{new HexagonStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		
		LOG("ChaosStage created, weak_ptr expired: %d", mChaosStage.expired());
		
		// Baðlantýyý burada yap - InitGameStages'den sonra GameHUD zaten oluþmuþ olacak
		ConnectChaosStageToHUD();
	}

	void LevelOne::ConnectChaosStageToHUD()
	{
		LOG("ConnectChaosStageToHUD called");
		LOG("mChaosStage.expired(): %d", mChaosStage.expired());
		
		if (auto chaosStage = mChaosStage.lock())
		{
			LOG("ChaosStage locked successfully");
			
			if (auto hud = GetGameHUD().lock())
			{
				LOG("GameHUD locked successfully - Connecting delegates");
				// hud zaten bir shared_ptr<GameHUD>, doðrudan GetWeakPtr() çaðrýlabilir
				chaosStage->onTotalChaosStarted.BindAction(hud->GetWeakPtr(), &GameHUD::TotalChaosStarted);
				chaosStage->onChaosTimerUpdated.BindAction(hud->GetWeakPtr(), &GameHUD::OnChaosTimerUpdated);
				chaosStage->onTotalChaosEnded.BindAction(hud->GetWeakPtr(), &GameHUD::TotalChaosEnded);
				LOG("Delegates connected successfully!");
			}
			else
			{
				LOG("ERROR: GameHUD not found or expired!");
			}
		}
		else
		{
			LOG("ERROR: ChaosStage not found or expired!");
		}
	}

	void LevelOne::Tick(float deltaTime)
	{
		// ? Her frame tüm yeni spawn edilmiþ actor'larý kontrol et
		// Eðer EnemySpaceShip ise event'e abone ol
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