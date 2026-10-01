#pragma once

#include "presentation/hud/vitals/VitalsViewModel.h"
#include "widget/Panel.h"
#include "widget/StackPanel.h"
#include "widget/ValueGauge.h"
#include "widget/ImageWidget.h"
#include "widget/TextWidget.h"

namespace ly
{
	class VitalsView : public Panel
	{
	public:
		explicit VitalsView(shared_ptr<const VitalsViewModel> viewModel);
		void Tick(float deltaTime) override;
		unsigned int GetRefreshCount() const { return mRefreshCount; }

	private:
		shared_ptr<const VitalsViewModel> mViewModel;
		UIRevisionWatcher mWatcher;
		weak_ptr<ValueGauge> mEnergyGauge;
		weak_ptr<ValueGauge> mShieldGauge;
		weak_ptr<ValueGauge> mHealthGauge;
		weak_ptr<TextWidget> mLifeText;
		weak_ptr<TextWidget> mScoreText;
		unsigned int mRefreshCount{ 0 };
	};
}
