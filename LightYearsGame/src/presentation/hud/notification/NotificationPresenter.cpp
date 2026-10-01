#include "presentation/hud/notification/NotificationPresenter.h"
#include "enemy/ChaosStage.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	NotificationPresenter::NotificationPresenter() : mViewModel{ std::make_shared<NotificationViewModel>() }
	{
	}

	void NotificationPresenter::BindChaosStage(const shared_ptr<ChaosStage>& stage)
	{
		mStageSubs.Clear();
		if (!stage) return;
		mStageSubs.Bind(stage, stage->onNotification, this, &NotificationPresenter::OnNotification);
		mStageSubs.Bind(stage, stage->onTotalChaosStarted, this, &NotificationPresenter::OnTimerStarted);
		mStageSubs.Bind(stage, stage->onChaosTimerUpdated, this, &NotificationPresenter::OnTimerUpdated);
		mStageSubs.Bind(stage, stage->onTotalChaosEnded, this, &NotificationPresenter::OnTimerEnded);
	}

	void NotificationPresenter::OnNotification(const std::string& text, float fadeIn, float hold, float fadeOut, const sf::Vector2f& location, float size, sf::Color color)
	{
		(void)location;
		NotificationRequest request{ text, fadeIn, hold, fadeOut, static_cast<unsigned int>(std::max(1.f, size)), color, mNextNotificationId++ };
		auto pending = mViewModel->pending;
		pending.push_back(std::move(request));
		if (pending.size() > 8) pending.erase(pending.begin());
		SetIfChanged(mViewModel->pending, pending, mViewModel->revision);
	}

	void NotificationPresenter::OnTimerStarted(float fadeIn, float hold, float fadeOut)
	{
		(void)hold;
		(void)fadeOut;
		SetIfChanged(mViewModel->timerFadeIn, fadeIn, mViewModel->revision);
		SetIfChanged(mViewModel->timerVisible, true, mViewModel->revision);
		SetIfChanged(mViewModel->timerActivation, mViewModel->timerActivation + 1, mViewModel->revision);
	}

	void NotificationPresenter::OnTimerUpdated(float timeLeft)
	{
		const int seconds = static_cast<int>(std::ceil(timeLeft));
		SetIfChanged(mViewModel->timerSeconds, seconds, mViewModel->revision);
	}

	void NotificationPresenter::OnTimerEnded()
	{
		SetIfChanged(mViewModel->timerVisible, false, mViewModel->revision);
	}
}
