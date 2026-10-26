#pragma once

#include "presentation/hud/weaponChoice/WeaponChoicePresenter.h"
#include "widget/Button.h"
#include "widget/Panel.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

#include <memory>
#include <vector>

namespace vega
{
	// Modal list of the Second Hardpoint weapon options. A click or the number key of a row picks that weapon.
	class WeaponChoiceView : public Panel
	{
	public:
		WeaponChoiceView(EngineContext& context, std::shared_ptr<const WeaponChoiceViewModel> viewModel, std::weak_ptr<WeaponChoicePresenter> presenter);
		void Tick(float deltaTime) override;
		std::size_t GetOptionButtonCount() const { return mButtons.size(); }

	private:
		// Click target of one row. Targets are kept for the view's lifetime (rows are only re-labelled), so a button that is
		// still being removed never points at a destroyed target.
		struct OptionTarget
		{
			WeaponChoiceView* view = nullptr;
			std::size_t index = 0;
			void OnClicked() { view->Choose(index); }
		};
		void Choose(std::size_t index);
		void Rebuild();

		std::shared_ptr<const WeaponChoiceViewModel> mViewModel;
		std::weak_ptr<WeaponChoicePresenter> mPresenter;
		UIRevisionWatcher mWatcher;
		weak_ptr<TextWidget> mTitle;
		weak_ptr<StackPanel> mColumn;
		std::vector<weak_ptr<Button>> mButtons;
		std::vector<std::unique_ptr<OptionTarget>> mTargets;
	};
}
