#pragma once

#include "presentation/hud/warning/GameplayWarningViewModel.h"
#include "widget/Panel.h"
#include "widget/TextWidget.h"

namespace ly
{
	struct HUDMigrationWarningTestAccess;

	class GameplayWarningView : public Panel
	{
		friend struct HUDMigrationWarningTestAccess;

	public:
		explicit GameplayWarningView(shared_ptr<const GameplayWarningViewModel> viewModel);
		void Tick(float deltaTime) override;

	private:
		shared_ptr<const GameplayWarningViewModel> mViewModel;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mText;
		UILayout mBaseLayout;
		std::uint32_t mActivation{ 0 };
		float mAnimTime{ 0.f };
	};
}
