#pragma once

#include "framework/SubscriptionSet.h"
#include "presentation/hud/vitals/VitalsViewModel.h"
#include <memory>

namespace ly
{
	class Player;

	class VitalsPresenter
	{
	public:
		VitalsPresenter();
		~VitalsPresenter() = default;
		VitalsPresenter(const VitalsPresenter&) = delete;
		VitalsPresenter& operator=(const VitalsPresenter&) = delete;
		VitalsPresenter(VitalsPresenter&&) = delete;
		VitalsPresenter& operator=(VitalsPresenter&&) = delete;

		void Tick();
		shared_ptr<const VitalsViewModel> GetViewModel() const { return mViewModel; }

	private:
		void OnPlayerCreated(Player* player);
		void OnPlayerAboutToBeDestroyed(Player* player);
		void OnLifeChanged(int life);
		void OnScoreChanged(int score);
		void BindPlayer(Player* player);
		void ClearShipValues();

		shared_ptr<VitalsViewModel> mViewModel;
		SubscriptionSet mManagerSubs;
		SubscriptionSet mPlayerSubs;
		unsigned int mBoundPlayerId{ 0 };
		unsigned int mPlayerBeingDestroyedId{ 0 };
		bool mHasBoundPlayer{ false };
		bool mHasPlayerBeingDestroyed{ false };
	};
}
