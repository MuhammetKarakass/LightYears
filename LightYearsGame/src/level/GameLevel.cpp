#include "level/GameLevel.h"
#include "widget/GameHUD.h"
#include "widget/PauseMenuHUD.h"
#include "framework/Application.h"
#include "level/MainMenuLevel.h"
#include "level/LevelOne.h"
#include "player/PlayerManager.h"

namespace ly
{
	GameLevel::GameLevel(Application* owningApp):
		World(owningApp)
	{
	}
	void GameLevel::BeginPlay()
	{
		mGameHUD = SpawnHUD<GameHUD>();
		OnGameStart();
	}

	bool GameLevel::DispatchEvent(const sf::Event& event)
	{
		// ✅ ESC tuşu kontrolü
		if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
		{
			if (keyPressed->code == sf::Keyboard::Key::Escape)
			{
				TogglePause();
				return true;
			}
		}
		
		// ✅ Diğer event'leri base class'a ilet
		return World::DispatchEvent(event);
	}
	
	void GameLevel::OnGameStart()
	{
	}
	void GameLevel::OnGamePaused()
	{
	}
	void GameLevel::OnGameResumed()
	{
	}
	void GameLevel::OnResumeGame()
	{
		mTimerHandle = TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), &GameLevel::TogglePause, 0.1f, false);
	}
	void GameLevel::OnQuitToMenu()
	{
		TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);

		if(IsPaused())
			SetPaused(false);

		PlayerManager::GetPlayerManager().Reset();
		GetApplication()->LoadWorld<MainMenuLevel>();
	}
	void GameLevel::OnRestartLevel()
	{
		TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);

		if(IsPaused())
			SetPaused(false);

		PlayerManager::GetPlayerManager().Reset();
	}
	void GameLevel::OnQuitGame()
	{
		GetApplication()->QuitApplication();
	}

	void GameLevel::TogglePause()
	{
		TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);
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
}