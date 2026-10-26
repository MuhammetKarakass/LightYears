#include "presentation/hud/weaponChoice/WeaponChoiceView.h"
#include "widget/MenuLayout.h"

#include <string>
#include <utility>

namespace vega
{
	WeaponChoiceView::WeaponChoiceView(EngineContext& context, std::shared_ptr<const WeaponChoiceViewModel> viewModel, std::weak_ptr<WeaponChoicePresenter> presenter)
		: Panel{ context, { 1.f, 1.f } }, mViewModel{ std::move(viewModel) }, mPresenter{ std::move(presenter) }
	{
		SetLayout(UILayout::Stretch());
		SetBackgroundColor({ 10, 14, 24, 190 });
		mTitle = AddChild<TextWidget>("SECOND HARDPOINT - CHOOSE A WEAPON", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 28);
		if (auto title = mTitle.lock())
		{
			UILayout layout = UILayout::Anchored(UIAnchor::Top, { 0.f, 140.f });
			layout.pivot = { .5f, .5f };
			title->SetLayout(layout);
		}
		UILayout columnLayout = UILayout::Anchored(UIAnchor::Top, { 0.f, 220.f });
		columnLayout.pivot = { .5f, 0.f };
		mColumn = BuildMenuColumn(*this, columnLayout, 18.f);
		SetVisibility(false);
	}

	void WeaponChoiceView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel || !mWatcher.Consume(mViewModel->revision)) return;
		SetVisibility(mViewModel->open);
		Rebuild();
	}

	void WeaponChoiceView::Rebuild()
	{
		const auto column = mColumn.lock();
		if (!column) return;
		for (const auto& button : mButtons)
			if (auto locked = button.lock()) locked->DestroyWidget();
		mButtons.clear();
		if (!mViewModel->open) return;
		for (std::size_t index = 0; index < mViewModel->options.size(); ++index)
		{
			if (mTargets.size() <= index) mTargets.push_back(std::make_unique<OptionTarget>(OptionTarget{ this, index }));
			auto button = AddMenuButton(*column, std::to_string(index + 1) + "  " + mViewModel->options[index].label, 22);
			if (auto locked = button.lock()) locked->onButtonClicked.BindAction(mTargets[index].get(), &OptionTarget::OnClicked);
			mButtons.push_back(button);
		}
	}

	void WeaponChoiceView::Choose(std::size_t index)
	{
		if (auto presenter = mPresenter.lock()) presenter->Choose(index);
	}
}
