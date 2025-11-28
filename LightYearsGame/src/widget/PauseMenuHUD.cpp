#include "widget/PauseMenuHUD.h"

namespace ly
{
	PauseMenuHUD::PauseMenuHUD():
		mTitleText{"Paused"},
		mResumeButton{"Resume"},
		mRestartButton{"Restart"},
		mQuitButton{"Quit"},
		mMainMenuButton{"Main Menu"}
	{
		mTitleText.SetTextSize(40);
		mResumeButton.SetTextSize(25);
		mRestartButton.SetTextSize(25);
		mQuitButton.SetTextSize(20);
		mMainMenuButton.SetTextSize(20);
	}
	
	void PauseMenuHUD::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mOverlay);

		mTitleText.NativeDraw(windowRef);
		mResumeButton.NativeDraw(windowRef);
		mRestartButton.NativeDraw(windowRef);
		mQuitButton.NativeDraw(windowRef);
		mMainMenuButton.NativeDraw(windowRef);
	}

	bool PauseMenuHUD::HandleEvent(const sf::Event& event)
	{
		if(const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
		{
			if(keyPressed->code == sf::Keyboard::Key::Escape)
			{
				onResumeButtonClicked.Broadcast();
				return true;
			}
		}

		bool handled = false;
		handled = mResumeButton.HandleEvent(event) || handled;
		handled = mRestartButton.HandleEvent(event) || handled;
		handled = mMainMenuButton.HandleEvent(event) || handled;
		handled = mQuitButton.HandleEvent(event) || handled;

		return handled || HUD::HandleEvent(event);
	}

	void PauseMenuHUD::Init(sf::RenderWindow& windowRef)
	{
		auto windowSize = windowRef.getSize();

		mOverlay.setSize(sf::Vector2f{static_cast<float>(windowSize.x)-100.f, static_cast<float>(windowSize.y)-100.f});
		mOverlay.setFillColor(sf::Color(60, 60, 60, 150));
		mOverlay.setOrigin(mOverlay.getSize() / 2.f);
		mOverlay.setPosition(sf::Vector2f{ static_cast<float>(windowSize.x) / 2.f, static_cast<float>(windowSize.y) / 2.f });

		mTitleText.CenterOrigin();
		mResumeButton.CenterOrigin();
		mQuitButton.CenterOrigin();
		mRestartButton.CenterOrigin();
		mMainMenuButton.CenterOrigin();

		mTitleText.SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, 100.f });
		mResumeButton.SetWidgetLocation(mTitleText.GetWidgetLocation() + sf::Vector2f{ 0.f, 100.f });
		mRestartButton.SetWidgetLocation(mResumeButton.GetWidgetLocation() + sf::Vector2f{ 0.f, 75.f });
		mQuitButton.SetWidgetLocation(mRestartButton.GetWidgetLocation() + sf::Vector2f{ 0.f, 75.f });
		mMainMenuButton.SetWidgetLocation(mQuitButton.GetWidgetLocation() + sf::Vector2f{ 0.f, 75.f });

		mResumeButton.onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::ResumeButtonClicked);
		mRestartButton.onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::RestartButtonClicked);
		mQuitButton.onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::QuitButtonClicked);
		mMainMenuButton.onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::MainMenuButtonClicked);

	}
	void PauseMenuHUD::ResumeButtonClicked()
	{
		onResumeButtonClicked.Broadcast();
	}
	void PauseMenuHUD::RestartButtonClicked()
	{
		onRestartButtonClicked.Broadcast();
	}
	void PauseMenuHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}
	void PauseMenuHUD::MainMenuButtonClicked()
	{
		onMainMenuButtonClicked.Broadcast();
	}
}