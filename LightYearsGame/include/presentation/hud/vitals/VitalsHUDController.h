#pragma once

#include "presentation/hud/HUDController.h"
#include "presentation/hud/vitals/VitalsPresenter.h"

namespace ly
{
	class GameHUD;
	class VitalsView;

	class VitalsHUDController : public HUDController
	{
	public:
		explicit VitalsHUDController(weak_ptr<GameHUD> gameHUD);
		~VitalsHUDController() override;
		void Tick(float deltaTime) override;

	private:
		weak_ptr<GameHUD> mGameHUD;
		VitalsPresenter mPresenter;
		weak_ptr<VitalsView> mView;
	};
}
