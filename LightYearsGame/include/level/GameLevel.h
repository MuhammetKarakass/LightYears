#pragma once

#include "framework/World.h"
#include "framework/TimerManager.h"


namespace ly
{

	class Application;
	class GameHUD;
	class PauseMenuHUD;
	class GameLevel : public World
	{
		public:
			GameLevel(Application* owningApp);

			weak_ptr<GameHUD> GetGameHUD()const { return mGameHUD; };
			weak_ptr<PauseMenuHUD> GetPauseMenuHUD()const { return mPauseMenuHUD; };

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


	private:
		void TogglePause();
		void ShowPauseMenu();
		void HidePauseMenu();

		weak_ptr<GameHUD> mGameHUD;
		weak_ptr<PauseMenuHUD> mPauseMenuHUD;

		bool mIsGamePausedToggle = false;

		TimerHandle mTimerHandle;
	};
}