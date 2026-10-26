#include "presentation/hud/vitals/VitalsView.h"
#include "widget/UIStyle.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace vega
{
	VitalsView::VitalsView(EngineContext& context, shared_ptr<const VitalsViewModel> viewModel)
		: Panel{ context, { 510.f, 92.f } }, mViewModel{ std::move(viewModel) }
	{
		const UIStyle& style = GetEngineContext().Style();
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
		mGauges = gauges;
		if (auto layout = gauges.lock()) layout->SetPanelSize({ 220.f, 78.f });
		mEnergyGauge = gauges.lock()->AddChild<ValueGauge>(sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 70, 205, 255, 255 }, sf::Color{ 35, 70, 95, 255 });
		mBlinkInfo = mEnergyGauge.lock()->AddChild<TextWidget>(" ", style.Font(UIFontRole::Body), 12u);
		mBlinkInfo.lock()->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, { 0.f, -34.f }));
		mBlinkInfo.lock()->SetVisibility(false);
		mShieldGauge = gauges.lock()->AddChild<ValueGauge>(sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 120, 180, 255, 255 }, sf::Color{ 35, 55, 95, 255 });
		mReserveTrack = mShieldGauge.lock()->AddChild<Panel>(sf::Vector2f{ 0.f, 6.f });
		mReserveTrack.lock()->SetBackgroundColor(sf::Color{ 40, 70, 60, 255 });
		mReserveFill = mReserveTrack.lock()->AddChild<Panel>(sf::Vector2f{ 0.f, 6.f });
		mReserveFill.lock()->SetBackgroundColor(sf::Color{ 90, 245, 185, 255 });
		mReserveTrack.lock()->SetVisibility(false);
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

		if (auto gauge = mEnergyGauge.lock()) gauge->UpdateValue(mViewModel->blinkDrive ? static_cast<float>(mViewModel->blinkCharges) : mViewModel->energy, mViewModel->blinkDrive ? static_cast<float>(mViewModel->blinkMaximumCharges) : mViewModel->energyMax);
		if (auto text = mBlinkInfo.lock())
		{
			text->SetVisibility(mViewModel->blinkDrive);
			std::ostringstream label;
			label << "Blink Drive";
			if (mViewModel->blinkRechargeRemaining > 0.f) label << " | next " << std::fixed << std::setprecision(1) << mViewModel->blinkRechargeRemaining << "s";
			label << "\nAB capacity/regen inactive";
			text->SetString(label.str());
		}
		if (auto gauge = mShieldGauge.lock()) gauge->UpdateValue(mViewModel->shield, mViewModel->shieldMax);
		const bool showReserve = mViewModel->hasShip && mViewModel->shieldReserveCap > 0.f && mViewModel->shield >= mViewModel->shieldMax;
		const float capWidth = 220.f * mViewModel->shieldReserveCap / std::max(1.f, mViewModel->shieldMax);
		if (auto track = mReserveTrack.lock())
		{
			track->SetVisibility(showReserve);
			track->SetPanelSize({ capWidth, 6.f });
			track->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, { 224.f, 6.f }, { capWidth, 6.f }));
		}
		if (auto fill = mReserveFill.lock())
		{
			const float width = capWidth * std::clamp(mViewModel->shieldReserve / std::max(.001f, mViewModel->shieldReserveCap), 0.f, 1.f);
			fill->SetPanelSize({ width, 6.f });
			fill->SetLayout(UILayout::Anchored(UIAnchor::TopLeft, {}, { width, 6.f }));
		}
		if (auto gauges = mGauges.lock()) gauges->SetPanelSize({ 220.f + (showReserve ? capWidth + 4.f : 0.f), 78.f });
		if (auto gauge = mHealthGauge.lock())
		{
			gauge->SetVisibility(mViewModel->healthReadoutVisible);
			gauge->UpdateValue(mViewModel->displayHealth, mViewModel->displayHealthMax);
			gauge->SetForegroundColor(ComputeHealthBarColor(*mViewModel));
		}
		if (auto text = mLifeText.lock()) text->SetString(mViewModel->hasPlayer ? std::to_string(mViewModel->life) : "0");
		if (auto text = mScoreText.lock()) text->SetString(mViewModel->hasPlayer ? std::to_string(mViewModel->score) : "0");
		++mRefreshCount;
	}
}
