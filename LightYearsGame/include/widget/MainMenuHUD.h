#pragma once

#include "widget/HUD.h"
#include "widget/Button.h"

namespace ly
{
	class MainMenuHUD : public HUD
	{
	public:
		MainMenuHUD();
		Delegate<> onStartButtonClicked;
		Delegate<> onQuitButtonClicked;

	private:
		void Init(sf::RenderWindow& windowRef) override;
		void StartButtonClicked();
		void QuitButtonClicked();

		weak_ptr<Button> mStartButton;
		weak_ptr<Button> mQuitButton;
	};
}
