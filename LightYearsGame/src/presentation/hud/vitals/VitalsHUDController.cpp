#include "presentation/hud/vitals/VitalsHUDController.h"
#include "presentation/hud/vitals/VitalsView.h"
#include "widget/GameHUD.h"

namespace ly
{
	VitalsHUDController::VitalsHUDController(weak_ptr<GameHUD> gameHUD) : mGameHUD{ gameHUD }
	{
	}

	VitalsHUDController::~VitalsHUDController()
	{
		if (auto view = mView.lock()) view->DestroyWidget();
	}

	void VitalsHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;
		mPresenter.Tick();
		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<VitalsView>(UILayer::Hud, mPresenter.GetViewModel());
	}
}
