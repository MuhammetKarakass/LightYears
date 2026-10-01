#include "presentation/hud/vitals/VitalsView.h"
#include "widget/UIStyle.h"

namespace ly
{
	VitalsView::VitalsView(shared_ptr<const VitalsViewModel> viewModel)
		: Panel{ { 510.f, 92.f } }, mViewModel{ std::move(viewModel) }
	{
		const UIStyle& style = UIStyle::Get();
		SetBackgroundColor(sf::Color::Transparent);
		SetLayout(UILayout::Anchored(UIAnchor::BottomLeft, { 20.f, -20.f }));

		auto composition = AddChild<StackPanel>(UIOrientation::Horizontal, 10.f);
		if (auto layout = composition.lock())
		{
			layout->SetPanelSize({ 510.f, 92.f });
			layout->SetCrossAlign(UIAlign::End);
			layout->SetLayout(UILayout::Stretch());
		}

		auto gauges = composition.lock()->AddChild<StackPanel>(UIOrientation::Vertical, 6.f);
		if (auto layout = gauges.lock()) layout->SetPanelSize({ 220.f, 78.f });
		mEnergyGauge = gauges.lock()->AddChild<ValueGauge>(sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 70, 205, 255, 255 }, sf::Color{ 35, 70, 95, 255 });
		mShieldGauge = gauges.lock()->AddChild<ValueGauge>(sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 120, 180, 255, 255 }, sf::Color{ 35, 55, 95, 255 });
		mHealthGauge = gauges.lock()->AddChild<ValueGauge>(sf::Vector2f{ 220.f, 30.f }, 1.f, sf::Color{ 128, 255, 128, 255 }, style.Color(UIColorRole::GaugeBackground));

		auto status = composition.lock()->AddChild<StackPanel>(UIOrientation::Horizontal, 10.f);
		if (auto layout = status.lock()) layout->SetCrossAlign(UIAlign::Center);
		status.lock()->AddChild<ImageWidget>("SpaceShooterRedux/PNG/pickups/playerLife1_blue.png");
		mLifeText = status.lock()->AddChild<TextWidget>(" ", style.Font(UIFontRole::Body), style.TextSize(UITextSize::Large));
		status.lock()->AddChild<ImageWidget>("SpaceShooterRedux/PNG/Power-ups/star_gold.png");
		mScoreText = status.lock()->AddChild<TextWidget>(" ", style.Font(UIFontRole::Body), style.TextSize(UITextSize::Large));
	}

	void VitalsView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel || !mWatcher.Consume(mViewModel->revision)) return;

		if (auto gauge = mEnergyGauge.lock()) gauge->UpdateValue(mViewModel->energy, mViewModel->energyMax);
		if (auto gauge = mShieldGauge.lock()) gauge->UpdateValue(mViewModel->shield, mViewModel->shieldMax);
		if (auto gauge = mHealthGauge.lock())
		{
			gauge->UpdateValue(mViewModel->displayHealth, mViewModel->displayHealthMax);
			gauge->SetForegroundColor(ComputeHealthBarColor(*mViewModel));
		}
		if (auto text = mLifeText.lock()) text->SetString(mViewModel->hasPlayer ? std::to_string(mViewModel->life) : "0");
		if (auto text = mScoreText.lock()) text->SetString(mViewModel->hasPlayer ? std::to_string(mViewModel->score) : "0");
		++mRefreshCount;
	}
}
