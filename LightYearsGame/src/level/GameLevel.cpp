#include "level/GameLevel.h"
#include "widget/GameHUD.h"
#include "widget/PauseMenuHUD.h"
#include "widget/GameOverHUD.h"
#include "framework/Application.h"
#include "framework/AudioManager.h"
#include "level/MainMenuLevel.h"
#include "level/LevelOne.h"
#include "player/PlayerManager.h"
#include "enemy/EnemyActor.h"
#include "presentation/hud/GameplayWarningHUDController.h"
#include "presentation/hud/ability/AbilityBarHUDController.h"
#include "presentation/hud/vitals/VitalsHUDController.h"
#include "presentation/hud/modules/ModuleScreenHUDController.h"
#include "presentation/hud/notification/NotificationHUDController.h"

namespace vega
{
	// Headless levels (tests, validation) have no application; they use the bound scope explicitly.
	GameLevel::GameLevel(Application* owningApp):
		World(owningApp, owningApp ? nullptr : EngineContext::TryCurrent()),
		mHUDControllers{}
	{
	}

	void GameLevel::BeginPlay()
	{
		World::BeginPlay();

		InitializeLevelSystems();
		CreateGameHUD();
		CreateHUDControllers();
		OnGameStart();

		
	}

	void GameLevel::Tick(float deltaTime)
	{
		for (const shared_ptr<HUDController>& controller : mHUDControllers)
		{
			if (controller)
			{
				controller->Tick(deltaTime);
			}
		}
		World::Tick(deltaTime);
		if (Player* player = PlayerManager::GetPlayerManager().GetPlayer())
		{
			player->FlushPendingTestAbilityLevels(this);
		}
	}

	bool GameLevel::DispatchEvent(const sf::Event& event)
	{
		if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
		{
			if (keyPressed->code == sf::Keyboard::Key::Escape)
			{
				if(!mGameOverHUD.expired())
					return true;

				TogglePause();
				return true;
			}
		}

		// The paused world and overlay menus own all input; controllers only see a running game.
		if (!IsPaused())
		{
			for (const shared_ptr<HUDController>& controller : mHUDControllers)
				if (controller && controller->HandleEvent(event)) return true;
		}
		
		return World::DispatchEvent(event);
	}

	void GameLevel::OnActorSpawned(Actor* actor)
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player)
		{
			return;
		}

		if (EnemyActor* enemy = dynamic_cast<EnemyActor*>(actor))
		{
			enemy->onScoreAwarded.BindAction(player, &Player::OnScoreAwarded);
			enemy->onShipXPAwarded.BindAction(player, &Player::AwardShipXP);
			return;
		}
	}
	
	void GameLevel::OnGameStart()
	{
		// Level music is off: it kept playing after Stop in the editor.
	}

	void GameLevel::OnGamePaused()
	{
		GetEngineContext().Audio().SetMenuMode(true);
	}

	void GameLevel::OnGameResumed()
	{
		GetEngineContext().Audio().SetMenuMode(false);
	}

	void GameLevel::InitializeLevelSystems()
	{
	}

	void GameLevel::CreateGameHUD()
	{
		mGameHUD = SpawnHUD<GameHUD>();
	}

	void GameLevel::CreateHUDControllers()
	{
		AddHUDController(std::make_shared<GameplayWarningHUDController>(mGameHUD));
		AddHUDController(std::make_shared<AbilityBarHUDController>(mGameHUD));
		AddHUDController(std::make_shared<VitalsHUDController>(mGameHUD));
		AddHUDController(std::make_shared<ModuleScreenHUDController>(mGameHUD));
		auto notificationController = std::make_shared<NotificationHUDController>(mGameHUD);
		mNotificationHUD = notificationController;
		AddHUDController(std::move(notificationController));
	}

	void GameLevel::AddHUDController(shared_ptr<HUDController> controller)
	{
		if (controller) mHUDControllers.push_back(std::move(controller));
	}

	void GameLevel::BroadcastGameplayWarning(const GameplayWarning& warning)
	{
		for (const shared_ptr<HUDController>& controller : mHUDControllers)
		{
			if (controller)
			{
				controller->ShowGameplayWarning(warning);
			}
		}
	}

	void GameLevel::ClearGameplayWarning(GameplayWarningType warningType)
	{
		for (const shared_ptr<HUDController>& controller : mHUDControllers)
		{
			if (controller)
			{
				controller->HideGameplayWarning(warningType);
			}
		}
	}

	void GameLevel::OnResumeGame()
	{
		mTimerHandle = GetEngineContext().GlobalTimers().SetTimer(GetWeakPtr(), &GameLevel::TogglePause, 0.1f, false);
	}

	void GameLevel::OnQuitToMenu()
	{
		mTimerHandle = GetEngineContext().GlobalTimers().SetTimer(GetWeakPtr(), [this]()
			{
				GetEngineContext().Audio().SetMenuMode(false);
				GetEngineContext().GlobalTimers().ClearTimer(mTimerHandle);

				if (IsPaused())
					SetPaused(false);

				GetEngineContext().Audio().StopMusic();

				PlayerManager::GetPlayerManager().Reset();
				GetApplication()->LoadWorld<MainMenuLevel>();
			}, 0.1f, false);
	}

	void GameLevel::OnRestartLevel()
	{
		GetEngineContext().Audio().SetMenuMode(false);
		PlayerManager::GetPlayerManager().Reset();
		mTimerHandle = GetEngineContext().GlobalTimers().SetTimer(GetWeakPtr(), [this]()
			{
				GetEngineContext().GlobalTimers().ClearTimer(mTimerHandle);

				if (IsPaused())
					SetPaused(false);

			}, 0.1f, false);
	}

	void GameLevel::OnQuitGame()
	{
		mTimerHandle= GetEngineContext().GlobalTimers().SetTimer(GetWeakPtr(), [this]()
		{
			GetEngineContext().Audio().SetMenuMode(false);
			GetEngineContext().GlobalTimers().ClearTimer(mTimerHandle);
			if (IsPaused())
				SetPaused(false);
			GetApplication()->QuitApplication();
			}, 0.1f, false);
	}

	void GameLevel::TogglePause()
	{
		GetEngineContext().GlobalTimers().ClearTimer(mTimerHandle);
		if (IsPaused())
		{
			SetPaused(false);
			HidePauseMenu();
			OnGameResumed();
		}
		else
		{
			SetPaused(true);
			ShowPauseMenu();
			OnGamePaused();
		}
	}

	void GameLevel::ShowPauseMenu()
	{
		mPauseMenuHUD = SpawnOverlayHUD<PauseMenuHUD>();

		if (auto pauseMenu = mPauseMenuHUD.lock())
		{
			pauseMenu->onResumeButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnResumeGame);
			pauseMenu->onMainMenuButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnQuitToMenu);
			pauseMenu->onQuitButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnQuitGame);
			pauseMenu->onRestartButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnRestartLevel);
		}
	}

	void GameLevel::HidePauseMenu()
	{
		RemoveOverlayHUD();
		mPauseMenuHUD.reset();
	}

	void GameLevel::AllGameStagesFinished()
	{
		GameFinished(true);
	}

	void GameLevel::GameFinished(bool playerWon)
	{
		if(mGameOverHUD.lock())
		{
			return;
		}

		mGameOverHUD = SpawnOverlayHUD<GameOverHUD>();

		if(auto hud= mGameOverHUD.lock())
		{
			hud->SetTitleText(playerWon ? "You Win!" : "Game Over");

			if (auto player = PlayerManager::GetPlayerManager().GetPlayer())
			{
				hud->SetScoreText(player->GetScore());
			}
			else
			{
				hud->SetScoreText(0);
			}

			hud->onMainMenuButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnQuitToMenu);
			hud->onQuitButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnQuitGame);
			hud->onRestartButtonClicked.BindAction(GetWeakPtr(), &GameLevel::OnRestartLevel);
		}
		SetPaused(true);
		OnGamePaused();
	}

	void GameLevel::GameOver()
	{
		GameFinished(false);
	}
}


