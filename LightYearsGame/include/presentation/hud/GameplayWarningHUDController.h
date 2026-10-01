#pragma once

#include "framework/Core.h"
#include "presentation/hud/HUDController.h"
#include "presentation/hud/warning/GameplayWarningViewModel.h"

namespace ly
{
	struct HUDMigrationWarningTestAccess;
	class GameHUD;
	class GameplayWarningView;

	class GameplayWarningHUDController : public HUDController
	{
		friend struct HUDMigrationWarningTestAccess;

	public:
		GameplayWarningHUDController(weak_ptr<GameHUD> gameHUD);
		~GameplayWarningHUDController() override;
		void Tick(float deltaTime) override;

		virtual void ShowGameplayWarning(const GameplayWarning& warning) override;
		virtual void HideGameplayWarning(GameplayWarningType warningType) override;

	private:
		weak_ptr<GameHUD> mGameHUD;
		shared_ptr<GameplayWarningViewModel> mViewModel;
		weak_ptr<GameplayWarningView> mView;
	};
}


