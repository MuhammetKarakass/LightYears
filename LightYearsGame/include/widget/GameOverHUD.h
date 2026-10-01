#pragma once

#include "widget/HUD.h"
#include "widget/Button.h"

namespace ly
{
	class TextWidget;

	class GameOverHUD : public HUD
	{
	public:
		GameOverHUD();
		void SetTitleText(const std::string& titleText);
		void SetScoreText(unsigned int score);

		Delegate<> onRestartButtonClicked;
		Delegate<> onQuitButtonClicked;
		Delegate<> onMainMenuButtonClicked;

	private:
		void Init(sf::RenderWindow& windowRef) override;
		void RestartButtonClicked();
		void QuitButtonClicked();
		void MainMenuButtonClicked();

		weak_ptr<TextWidget> mTitleText;
		weak_ptr<TextWidget> mScoreText;
		weak_ptr<Button> mRestartButton;
		weak_ptr<Button> mQuitButton;
		weak_ptr<Button> mMainMenuButton;
	};
}
