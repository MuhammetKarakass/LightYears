#pragma once

#include "widget/HUD.h"
#include "widget/Button.h"
#include "widget/TextWidget.h"

namespace ly
{
	class PauseMenuHUD : public HUD
	{
	public:
		PauseMenuHUD();

		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual bool HandleEvent(const sf::Event& event) override;

		Delegate<> onResumeButtonClicked;
		Delegate<> onRestartButtonClicked;
		Delegate<> onQuitButtonClicked;
		Delegate<> onMainMenuButtonClicked;

	private:
		virtual void Init(sf::RenderWindow& windorRef) override;
		void ResumeButtonClicked();
		void RestartButtonClicked();
		void QuitButtonClicked();
		void MainMenuButtonClicked();

		TextWidget mTitleText;
		Button mResumeButton;
		Button mRestartButton;
		Button mQuitButton;
		Button mMainMenuButton;

		sf::RectangleShape mOverlay;
	};
}