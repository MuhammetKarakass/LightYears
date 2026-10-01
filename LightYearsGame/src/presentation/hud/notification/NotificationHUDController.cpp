#include "presentation/hud/notification/NotificationHUDController.h"
#include "presentation/hud/notification/NotificationView.h"
#include "widget/GameHUD.h"

namespace ly
{
	NotificationHUDController::NotificationHUDController(weak_ptr<GameHUD> gameHUD) : mGameHUD{ gameHUD }
	{
	}

	NotificationHUDController::~NotificationHUDController()
	{
		if (auto view = mView.lock()) view->DestroyWidget();
	}

	void NotificationHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;
		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<NotificationView>(UILayer::Hud, mPresenter.GetViewModel());
	}
}
