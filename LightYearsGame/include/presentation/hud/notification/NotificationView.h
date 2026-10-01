#pragma once

#include "presentation/hud/notification/NotificationViewModel.h"
#include "widget/Panel.h"
#include "widget/TextWidget.h"

namespace ly
{
	class NotificationView : public Panel
	{
	public:
		explicit NotificationView(shared_ptr<const NotificationViewModel> viewModel);
		void Tick(float deltaTime) override;

	private:
		shared_ptr<const NotificationViewModel> mViewModel;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mTimerText;
		std::uint32_t mLastNotificationId{ 0 };
		std::uint32_t mTimerActivation{ 0 };
	};
}
