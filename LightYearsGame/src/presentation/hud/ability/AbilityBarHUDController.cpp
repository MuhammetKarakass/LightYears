#include "presentation/hud/ability/AbilityBarHUDController.h"

#include "presentation/hud/ability/AbilityBarView.h"
#include "widget/GameHUD.h"

namespace ly
{
	AbilityBarHUDController::AbilityBarHUDController(weak_ptr<GameHUD> gameHUD) : mGameHUD{ gameHUD }
	{
	}

	AbilityBarHUDController::~AbilityBarHUDController()
	{
		if (auto view = mView.lock()) view->DestroyWidget();
	}

	void AbilityBarHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;
		mPresenter.Tick();
		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<AbilityBarView>(UILayer::Hud, mPresenter.GetViewModel());
	}
}
