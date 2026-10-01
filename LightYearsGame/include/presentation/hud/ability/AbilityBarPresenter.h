#pragma once

#include "framework/SubscriptionSet.h"
#include "presentation/hud/ability/AbilityBarViewModel.h"
#include <memory>

namespace ly
{
	class PlayerSpaceShip;

	class AbilityBarPresenter
	{
	public:
		AbilityBarPresenter();
		AbilityBarPresenter(const AbilityBarPresenter&) = delete;
		AbilityBarPresenter& operator=(const AbilityBarPresenter&) = delete;
		AbilityBarPresenter(AbilityBarPresenter&&) = delete;
		AbilityBarPresenter& operator=(AbilityBarPresenter&&) = delete;

		void Tick();
		shared_ptr<const AbilityBarViewModel> GetViewModel() const { return mViewModel; }

	private:
		void BindShip(const shared_ptr<PlayerSpaceShip>& ship);
		void ClearShip();
		void OnAbilityChanged(sas::AbilityHandle handle);
		void OnAbilityLevelChanged(sas::AbilityHandle handle, int level);
		void RefreshStats();

		shared_ptr<AbilityBarViewModel> mViewModel;
		SubscriptionSet mShipSubs;
		weak_ptr<PlayerSpaceShip> mBoundShip;
		unsigned int mBoundShipId{ 0 };
		bool mHasBoundShip{ false };
		bool mStatsDirty{ true };
	};
}
