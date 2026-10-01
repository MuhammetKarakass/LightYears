#pragma once

#include "framework/SubscriptionSet.h"
#include "presentation/hud/notification/NotificationViewModel.h"
#include <SFML/System/Vector2.hpp>
#include <memory>

namespace ly
{
	class ChaosStage;

	class NotificationPresenter
	{
	public:
		NotificationPresenter();
		void BindChaosStage(const shared_ptr<ChaosStage>& stage);
		shared_ptr<const NotificationViewModel> GetViewModel() const { return mViewModel; }

	private:
		void OnNotification(const std::string& text, float fadeIn, float hold, float fadeOut, const sf::Vector2f& location, float size, sf::Color color);
		void OnTimerStarted(float fadeIn, float hold, float fadeOut);
		void OnTimerUpdated(float timeLeft);
		void OnTimerEnded();

		shared_ptr<NotificationViewModel> mViewModel;
		SubscriptionSet mStageSubs;
		std::uint32_t mNextNotificationId{ 1 };
	};
}
