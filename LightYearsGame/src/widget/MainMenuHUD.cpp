#include "widget/MainMenuHUD.h"

namespace ly
{
	MainMenuHUD::MainMenuHUD():
		mTitleText{"Light Years"},
		mStartButton{"Start"},
		mQuitButton{"Quit"}
	{
		mTitleText.SetTextSize(40);
		mStartButton.SetTextSize(25);
		mQuitButton.SetTextSize(20);
	}

	void MainMenuHUD::Draw(sf::RenderWindow& windowRef)
	{
		mTitleText.NativeDraw(windowRef);
		mStartButton.NativeDraw(windowRef);
		mQuitButton.NativeDraw(windowRef);
	}
	
	bool MainMenuHUD::HandleEvent(const sf::Event& event)
	{
		bool handled = false;
		handled = mStartButton.HandleEvent(event) || handled;
		handled = mQuitButton.HandleEvent(event) || handled;
		return handled || HUD::HandleEvent(event);
	}
	
	void MainMenuHUD::Init(sf::RenderWindow& windowRef)
	{
		auto windowSize = windowRef.getSize();
		
		// ✅ Origin'leri merkeze al
		mTitleText.CenterOrigin();
		mStartButton.CenterOrigin();
		mQuitButton.CenterOrigin();
		
		// Widget pozisyonları (artık merkezden konumlanacaklar)
		mTitleText.SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, 100.f });
		mStartButton.SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, windowSize.y / 2.f });
		mQuitButton.SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, windowSize.y / 2.f + 100.f });

		// Button event'leri
		mStartButton.onButtonClicked.BindAction(GetWeakPtr(), &MainMenuHUD::StartButtonClicked);
		mQuitButton.onButtonClicked.BindAction(GetWeakPtr(), &MainMenuHUD::QuitButtonClicked);
	}
	
	void MainMenuHUD::StartButtonClicked()
	{
		onStartButtonClicked.Broadcast();
	}

	void MainMenuHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}

}