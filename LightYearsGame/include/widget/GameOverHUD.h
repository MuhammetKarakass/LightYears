#pragma once

#include "widget/HUD.h"
#include "widget/TextWidget.h"
#include "widget/Button.h"

namespace ly
{
	class GameOverHUD : public HUD
	{
	public:
		GameOverHUD();

		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual bool HandleEvent(const sf::Event& event) override;

		void SetTitleText(const std::string& titleText);
		void SetScoreText(unsigned int score);

		Delegate<> onRestartButtonClicked;
		Delegate<> onQuitButtonClicked;
		Delegate<> onMainMenuButtonClicked;

	private:
		virtual void Init(sf::RenderWindow& windorRef) override;
		void RestartButtonClicked();
		void QuitButtonClicked();
		void MainMenuButtonClicked();
		TextWidget mTitleText;
		TextWidget mScoreText;
		Button mRestartButton;
		Button mQuitButton;
		Button mMainMenuButton;
		sf::RectangleShape mOverlay;
	};
}