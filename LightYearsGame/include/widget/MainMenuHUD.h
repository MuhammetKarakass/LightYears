#pragma once

#include "widget/HUD.h"
#include "widget/TextWidget.h"
#include "widget/Button.h"

namespace ly
{
	class MainMenuHUD : public HUD
	{
	public:
		MainMenuHUD();

		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual bool HandleEvent(const sf::Event& event) override;

		Delegate<> onStartButtonClicked;
		Delegate<> onQuitButtonClicked;
	private:
		virtual void Init(sf::RenderWindow& windowRef) override;

		void StartButtonClicked();
		void QuitButtonClicked();

		TextWidget mTitleText;
		Button mStartButton;
		Button mQuitButton;
	};
}