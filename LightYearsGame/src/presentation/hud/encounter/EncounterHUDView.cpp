#include "presentation/hud/encounter/EncounterHUDView.h"

namespace ly
{
	namespace
	{
		const char* const EncounterFontPath = "SpaceShooterRedux/Bonus/OrbitronBlack.ttf";
		const sf::Color DetailTextColor{ 150, 190, 220, 255 };
	}

	EncounterHUDView::EncounterHUDView(shared_ptr<const EncounterHUDPresentation> presentation)
		: Panel{ { 1.f, 1.f } }, mPresentation{ std::move(presentation) }
	{
		SetBackgroundColor(sf::Color::Transparent);
		SetLayout(UILayout::Stretch());
		auto lines = AddChild<StackPanel>(UIOrientation::Vertical, 8.f);
		if (auto layout = lines.lock())
		{
			layout->SetCrossAlign(UIAlign::Center);
			UILayout topLayout = UILayout::Anchored(UIAnchor::Top, { 0.f, 24.f });
			topLayout.pivot = { 0.5f, 0.f };
			layout->SetLayout(topLayout);
		}
		mTitle = lines.lock()->AddChild<TextWidget>("", EncounterFontPath, 24u);
		mDetail = lines.lock()->AddChild<TextWidget>("", EncounterFontPath, 17u);
	}

	void EncounterHUDView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mPresentation || !mWatcher.Consume(mPresentation->revision)) return;
		if (auto title = mTitle.lock())
		{
			title->SetString(mPresentation->title);
			title->SetFillColor(mPresentation->titleColor);
			title->SetVisibility(mPresentation->visible && !mPresentation->title.empty());
		}
		if (auto detail = mDetail.lock())
		{
			detail->SetString(mPresentation->detail);
			detail->SetFillColor(DetailTextColor);
			detail->SetVisibility(mPresentation->visible && !mPresentation->detail.empty());
		}
		SetVisibility(mPresentation->visible);
		++mRefreshCount;
	}
}
