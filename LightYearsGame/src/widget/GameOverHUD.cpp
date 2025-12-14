#include "widget/GameOverHUD.h"

namespace ly
{
	GameOverHUD::GameOverHUD() :
		mTitleText{ "Game Over" },
		mScoreText{ "Score: 0" },
		mRestartButton{ "Restart" },
		mQuitButton{ "Quit" },
		mMainMenuButton{ "Main Menu" }
	{
		mTitleText.SetTextSize(40);
		mScoreText.SetTextSize(30);
		mRestartButton.SetTextSize(25);
		mQuitButton.SetTextSize(20);
		mMainMenuButton.SetTextSize(20);
	}

	void GameOverHUD::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mOverlay);
		mTitleText.NativeDraw(windowRef);
		mScoreText.NativeDraw(windowRef);
		mRestartButton.NativeDraw(windowRef);
		mQuitButton.NativeDraw(windowRef);
		mMainMenuButton.NativeDraw(windowRef);
	}
	bool GameOverHUD::HandleEvent(const sf::Event& event)
	{
		bool handled = false;
		handled = mRestartButton.HandleEvent(event) || handled;
		handled = mQuitButton.HandleEvent(event) || handled;
		handled = mMainMenuButton.HandleEvent(event) || handled;
		return  handled || HUD::HandleEvent(event);
	}

	void GameOverHUD::Init(sf::RenderWindow& windowRef)
	{
		auto windowSize = windowRef.getSize();

		mOverlay.setSize(sf::Vector2f{ static_cast<float>(windowSize.x) - 100.f, static_cast<float>(windowSize.y) - 100.f });
		mOverlay.setFillColor(sf::Color(60, 60, 60, 150));
		mOverlay.setOrigin(mOverlay.getSize() / 2.f);
		mOverlay.setPosition(sf::Vector2f{ static_cast<float>(windowSize.x) / 2.f, static_cast<float>(windowSize.y) / 2.f });

		mTitleText.CenterOrigin();
		mScoreText.CenterOrigin();
		mRestartButton.CenterOrigin();
		mQuitButton.CenterOrigin();
		mMainMenuButton.CenterOrigin();

		mTitleText.SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, 150.f });
		mScoreText.SetWidgetLocation(mTitleText.GetWidgetLocation() + sf::Vector2f{ 0.f, 50.f });

		mRestartButton.SetWidgetLocation(mScoreText.GetWidgetLocation() + sf::Vector2f{ 0.f, 100.f });
		mMainMenuButton.SetWidgetLocation(mRestartButton.GetWidgetLocation() + sf::Vector2f{ 0.f, 75.f });
		mQuitButton.SetWidgetLocation(mMainMenuButton.GetWidgetLocation() + sf::Vector2f{ 0.f, 75.f });

		mRestartButton.onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::RestartButtonClicked);
		mQuitButton.onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::QuitButtonClicked);
		mMainMenuButton.onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::MainMenuButtonClicked);
	}

	void GameOverHUD::SetTitleText(const std::string& titleText)
	{
		mTitleText.SetString(titleText);
	}
	void GameOverHUD::SetScoreText(unsigned int score)
	{
		mScoreText.SetString("Score: " + std::to_string(score));
	}
	void GameOverHUD::RestartButtonClicked()
	{
		onRestartButtonClicked.Broadcast();
	}
	void GameOverHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}
	void GameOverHUD::MainMenuButtonClicked()
	{
		onMainMenuButtonClicked.Broadcast();
	}
}