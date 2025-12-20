#include "level/GameLevel.h"
#include "widget/GameHUD.h"
#include "widget/PauseMenuHUD.h"
#include "widget/GameOverHUD.h"
#include "framework/Application.h"
#include "framework/AudioManager.h"
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
		World::BeginPlay();

		mGameHUD = SpawnHUD<GameHUD>();
		OnGameStart();

		
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
		
		return World::DispatchEvent(event);
	}
	
	void GameLevel::OnGameStart()
	{
		AudioManager::GetAudioManager().PlayMusic(
			"SpaceShooterRedux/Musics/confetti1.ogg",
			AudioType::SFX_World,
			80.0f,  // Volume
			1.0f,   // Pitch
			true// Loop
		);
	}

	void GameLevel::OnGamePaused()
	{
		AudioManager::GetAudioManager().SetMenuMode(true);
	}

	void GameLevel::OnGameResumed()
	{
		AudioManager::GetAudioManager().SetMenuMode(false);
	}

	void GameLevel::OnResumeGame()
	{
		mTimerHandle = TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), &GameLevel::TogglePause, 0.1f, false);
	}

	void GameLevel::OnQuitToMenu()
	{
		mTimerHandle = TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), [this]()
			{
				AudioManager::GetAudioManager().SetMenuMode(false);
				TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);

				if (IsPaused())
					SetPaused(false);

				// Oyun müziğini durdur
				AudioManager::GetAudioManager().StopMusic();

				PlayerManager::GetPlayerManager().Reset();
				GetApplication()->LoadWorld<MainMenuLevel>();
			}, 0.1f, false);
	}

	void GameLevel::OnRestartLevel()
	{
		AudioManager::GetAudioManager().SetMenuMode(false);
		mTimerHandle = TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), [this]()
			{
				TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);

				if (IsPaused())
					SetPaused(false);

				PlayerManager::GetPlayerManager().Reset();
			}, 0.1f, false);
	}

	void GameLevel::OnQuitGame()
	{
		mTimerHandle= TimerManager::GetGlobalTimerManager().SetTimer(GetWeakPtr(), [this]()
		{
			AudioManager::GetAudioManager().SetMenuMode(false);
			TimerManager::GetGlobalTimerManager().ClearTimer(mTimerHandle);
			if (IsPaused())
				SetPaused(false);
			GetApplication()->QuitApplication();
			}, 0.1f, false);
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