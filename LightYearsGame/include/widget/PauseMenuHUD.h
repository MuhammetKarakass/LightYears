#pragma once

#include "widget/HUD.h"
#include "widget/Button.h"

namespace ly
{
	class PauseMenuHUD : public HUD
	{
	public:
		PauseMenuHUD();
		bool HandleEvent(const sf::Event& event) override;

		Delegate<> onResumeButtonClicked;
		Delegate<> onRestartButtonClicked;
		Delegate<> onQuitButtonClicked;
		Delegate<> onMainMenuButtonClicked;

	private:
		void Init(sf::RenderWindow& windowRef) override;
		void ResumeButtonClicked();
		void RestartButtonClicked();
		void QuitButtonClicked();
		void MainMenuButtonClicked();

		weak_ptr<Button> mResumeButton;
		weak_ptr<Button> mRestartButton;
		weak_ptr<Button> mQuitButton;
		weak_ptr<Button> mMainMenuButton;
	};
}
