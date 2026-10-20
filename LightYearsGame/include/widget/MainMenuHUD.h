#pragma once

#include "widget/HUD.h"
#include "widget/Button.h"

namespace vega
{
	class MainMenuHUD : public HUD
	{
	public:
		explicit MainMenuHUD(EngineContext& context);
		Delegate<> onStartButtonClicked;
		Delegate<> onQuitButtonClicked;

	private:
		void Init(sf::RenderTarget& windowRef) override;
		void StartButtonClicked();
		void QuitButtonClicked();

		weak_ptr<Button> mStartButton;
		weak_ptr<Button> mQuitButton;
	};
}
