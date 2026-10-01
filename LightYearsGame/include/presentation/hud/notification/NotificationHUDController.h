#pragma once

#include "presentation/hud/notification/NotificationPresenter.h"
#include "presentation/hud/HUDController.h"

namespace ly
{
	class GameHUD;
	class NotificationView;

	class NotificationHUDController : public HUDController
	{
	public:
		explicit NotificationHUDController(weak_ptr<GameHUD> gameHUD);
		~NotificationHUDController() override;
		void Tick(float deltaTime) override;
		void BindChaosStage(const shared_ptr<ChaosStage>& stage) { mPresenter.BindChaosStage(stage); }
		NotificationPresenter& GetPresenter() { return mPresenter; }

	private:
		weak_ptr<GameHUD> mGameHUD;
		NotificationPresenter mPresenter;
		weak_ptr<NotificationView> mView;
	};
}
