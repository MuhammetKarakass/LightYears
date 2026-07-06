#include "presentation/hud/GameplayWarningHUDController.h"
#include "widget/GameHUD.h"

namespace ly
{
	GameplayWarningHUDController::GameplayWarningHUDController(weak_ptr<GameHUD> gameHUD)
		: mGameHUD{ gameHUD }
	{
	}

	void GameplayWarningHUDController::ShowGameplayWarning(const GameplayWarning& warning)
	{
		if (auto hud = mGameHUD.lock())
		{
			hud->ShowGameplayWarning(warning);
		}
	}

	void GameplayWarningHUDController::HideGameplayWarning(GameplayWarningType warningType)
	{
		if (auto hud = mGameHUD.lock())
		{
			hud->HideGameplayWarning(warningType);
		}
	}
}
