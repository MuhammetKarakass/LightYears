#include "presentation/hud/notification/NotificationView.h"
#include <utility>

namespace ly
{
	NotificationView::NotificationView(shared_ptr<const NotificationViewModel> viewModel)
		: Panel{ { 1.f, 1.f } }, mViewModel{ std::move(viewModel) }
	{
		SetBackgroundColor(sf::Color::Transparent);
		SetLayout(UILayout::Stretch());
		mTimerText = AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 20);
		if (const shared_ptr<TextWidget> timer = mTimerText.lock())
		{
			timer->SetFillColor(sf::Color::Red);
			timer->SetVisibility(false);
			UILayout layout = UILayout::Anchored(UIAnchor::Top, { 0.f, 8.f });
			layout.pivot = { 0.5f, 0.f };
			timer->SetLayout(layout);
		}
	}

	void NotificationView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel || !mWatcher.Consume(mViewModel->revision)) return;
		for (const NotificationRequest& request : mViewModel->pending)
		{
			if (request.id <= mLastNotificationId) continue;
			auto text = AddChild<TextWidget>(request.text, "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", request.size).lock();
			if (text)
			{
				UILayout layout = UILayout::Anchored(UIAnchor::Center);
				layout.pivot = { 0.5f, 0.5f };
				text->SetLayout(layout);
				text->SetFillColor(request.color);
				text->StartFadeAnimation(request.fadeIn, request.hold, request.fadeOut);
				text->SetLifeTime(request.fadeIn + request.hold + request.fadeOut + 1.f);
			}
			mLastNotificationId = request.id;
		}
		if (const shared_ptr<TextWidget> timer = mTimerText.lock())
		{
			timer->SetString("TIME LEFT: " + std::to_string(mViewModel->timerSeconds));
			timer->SetVisibility(mViewModel->timerVisible);
			if (mTimerActivation != mViewModel->timerActivation)
			{
				mTimerActivation = mViewModel->timerActivation;
				timer->StartFadeAnimation(mViewModel->timerFadeIn, 0.f, 0.f);
			}
		}
	}
}
