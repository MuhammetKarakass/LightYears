#pragma once

#include "presentation/hud/HUDController.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"

namespace ly
{
	class GameHUD;
	class AbilityBarView;

	class AbilityBarHUDController : public HUDController
	{
	public:
		explicit AbilityBarHUDController(weak_ptr<GameHUD> gameHUD);
		~AbilityBarHUDController() override;
		void Tick(float deltaTime) override;

	private:
		weak_ptr<GameHUD> mGameHUD;
		AbilityBarPresenter mPresenter;
		weak_ptr<AbilityBarView> mView;
	};
}
