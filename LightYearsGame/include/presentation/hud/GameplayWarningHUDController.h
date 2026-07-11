#pragma once

#include "framework/Core.h"
#include "presentation/hud/HUDController.h"

namespace ly
{
	class GameHUD;

	class GameplayWarningHUDController : public HUDController
	{
	public:
		GameplayWarningHUDController(weak_ptr<GameHUD> gameHUD);

		virtual void ShowGameplayWarning(const GameplayWarning& warning) override;
		virtual void HideGameplayWarning(GameplayWarningType warningType) override;

	private:
		weak_ptr<GameHUD> mGameHUD;
	};
}


