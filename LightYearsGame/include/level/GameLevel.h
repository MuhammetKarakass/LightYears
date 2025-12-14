#pragma once

#include "framework/World.h"
#include "framework/TimerManager.h"


namespace ly
{

	class Application;
	class GameHUD;
	class PauseMenuHUD;
	class GameOverHUD;
	class GameLevel : public World
	{
		public:
			GameLevel(Application* owningApp);

			void GameFinished(bool playerWon);

			weak_ptr<GameHUD> GetGameHUD()const { return mGameHUD; };
			weak_ptr<PauseMenuHUD> GetPauseMenuHUD()const { return mPauseMenuHUD; };
			weak_ptr<GameOverHUD> GetGameOverHUD()const { return mGameOverHUD; };

	protected:
		virtual void BeginPlay() override;
		virtual bool DispatchEvent(const sf::Event& event) override;

		virtual void OnGameStart();
		virtual void OnGamePaused();
		virtual void OnGameResumed();

		virtual void OnResumeGame();
		virtual void OnQuitToMenu();
		virtual void OnRestartLevel();
		virtual void OnQuitGame();

		virtual void GameOver();


	private:
		void TogglePause();
		void ShowPauseMenu();
		void HidePauseMenu();

		virtual void AllGameStagesFinished() override;

		weak_ptr<GameHUD> mGameHUD;
		weak_ptr<PauseMenuHUD> mPauseMenuHUD;
		weak_ptr<GameOverHUD> mGameOverHUD;

		bool mIsGamePausedToggle = false;

		TimerHandle mTimerHandle;
	};
}