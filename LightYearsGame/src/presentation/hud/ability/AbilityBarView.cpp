#include "presentation/hud/ability/AbilityBarView.h"
#include "presentation/hud/modules/HoloDraw.h"

#include <cstdio>
#include <utility>

namespace vega
{
	namespace
	{
		weak_ptr<ImageWidget> CreateAbilityIcon(Panel& column, const std::string& path)
		{
			auto icon = column.AddChild<ImageWidget>(path);
			if (auto locked = icon.lock())
			{
				UILayout layout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, -68.f });
				layout.pivot = { 0.f, 0.f };
				locked->SetLayout(layout);
			}
			return icon;
		}
	}

	AbilityBarView::AbilityBarView(EngineContext& context, shared_ptr<const AbilityBarViewModel> viewModel)
		: Panel{ context, { 1.f, 1.f } }, mViewModel{ std::move(viewModel) }
	{
		SetBackgroundColor(sf::Color::Transparent);
		const UILayout rootLayout = UILayout::Anchored(UIAnchor::Bottom, { 50.f, -44.f });
		SetLayout(rootLayout);
		mWeaponReadout = AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 12);
		if (auto readout = mWeaponReadout.lock())
		{
			readout->SetLayout(UILayout::Anchored(UIAnchor::BottomLeft, { -620.f, -170.f }));
			readout->SetFillColor(sf::Color{ 220, 235, 255, 255 });
		}
		auto bar = AddChild<StackPanel>(UIOrientation::Horizontal, 28.f);
		if (auto locked = bar.lock())
		{
			locked->SetCrossAlign(UIAlign::End);
			locked->SetLayout(UILayout::Anchored(UIAnchor::Bottom));
		}

		if (mViewModel) mIconGeneration = mViewModel->shipGeneration;
		for (auto& group : mGroups) group = bar.lock()->AddChild<Panel>(sf::Vector2f{ 150.f, 200.f });
		for (std::size_t index = 0; index < mSlots.size(); ++index)
		{
			auto column = mGroups[index % 4].lock()->AddChild<Panel>(sf::Vector2f{ 150.f, 200.f });
			if (auto locked = column.lock())
			{
				UILayout layout;
				layout.size = { 150.f, 200.f };
				if (index >= 4)
				{
					layout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, -220.f });
					layout.pivot = { 0.f, 1.f };
					layout.size = { 150.f, 200.f };
				}
				locked->SetLayout(layout);
				locked->SetVisibility(false);
			}
			auto stats = column.lock()->AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 11);
			stats.lock()->SetFillColor(sf::Color{ 220, 235, 255, 255 });
			auto input = column.lock()->AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 14);
			weak_ptr<ImageWidget> icon;
			if (mViewModel && mViewModel->slots[index].visible && !mViewModel->slots[index].iconPath.empty())
			{
				mIconPaths[index] = mViewModel->slots[index].iconPath;
				icon = CreateAbilityIcon(*column.lock(), mIconPaths[index]);
			}
			auto state = column.lock()->AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 15);
			state.lock()->SetFillColor(sf::Color{ 120, 255, 140, 255 });
			UILayout statsLayout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, -94.f });
			statsLayout.pivot = { 0.f, 1.f };
			stats.lock()->SetLayout(statsLayout);
			UILayout inputLayout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, -88.f });
			inputLayout.pivot = { 0.f, 0.f };
			input.lock()->SetLayout(inputLayout);
			UILayout stateLayout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, -16.f });
			stateLayout.pivot = { 0.f, 0.f };
			state.lock()->SetLayout(stateLayout);
			mSlots[index] = { column, stats, input, icon, state, {} };
			mColumns[index] = column;
			mIcons[index] = icon;
		}
	}

	void AbilityBarView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel || !mWatcher.Consume(mViewModel->revision)) return;
		if (auto readout = mWeaponReadout.lock()) readout->SetString(mViewModel->weaponReadout);
		const bool generationChanged = mIconGeneration != mViewModel->shipGeneration;
		mIconGeneration = mViewModel->shipGeneration;
		for (std::size_t index = 0; index < mGroups.size(); ++index)
			if (auto group = mGroups[index].lock()) group->SetVisibility(mViewModel->slots[index].visible || mViewModel->slots[index + 4].visible);
		for (std::size_t index = 0; index < mSlots.size(); ++index)
		{
			const AbilitySlotViewData& data = mViewModel->slots[index];
			SlotWidgets& widgets = mSlots[index];
			if (auto column = widgets.column.lock()) column->SetVisibility(data.visible);
			if (!data.visible) continue;
			if (data.timing.open && widgets.timing.expired())
			{
				if (auto column = widgets.column.lock())
				{
					widgets.timing = column->AddChild<HoloCanvas>();
					if (auto canvas = widgets.timing.lock()) { auto layout = UILayout::Anchored(UIAnchor::BottomLeft, { 0.f, 4.f }); layout.pivot = { 0.f, 0.f }; layout.size = { 150.f, 18.f }; canvas->SetLayout(layout); }
				}
			}
			if (auto canvas = widgets.timing.lock())
			{
				canvas->SetVisibility(data.timing.open); canvas->Clear();
				if (data.timing.open)
				{
					auto& vertices = canvas->Normal();
					holo::Rect(vertices, { 0.f, 0.f }, { 150.f, 14.f }, { 28, 35, 49, 255 }, { 28, 35, 49, 255 });
					holo::Rect(vertices, { 150.f * data.timing.bigStart, 0.f }, { 150.f * (data.timing.bigEnd - data.timing.bigStart), 14.f }, { 65, 145, 180, 255 }, { 65, 145, 180, 255 });
					holo::Rect(vertices, { 150.f * data.timing.smallStart, 0.f }, { 150.f * (data.timing.smallEnd - data.timing.smallStart), 14.f }, { 100, 240, 135, 255 }, { 100, 240, 135, 255 });
					holo::Line(vertices, { data.timing.progress * 150.f, -3.f }, { data.timing.progress * 150.f, 17.f }, 3.f, sf::Color::White);
				}
			}
			if ((generationChanged || mIconPaths[index] != data.iconPath || widgets.icon.expired()) && !data.iconPath.empty())
			{
				// Recreate the sprite so its texture rectangle uses the new texture's intrinsic size.
				if (auto oldIcon = widgets.icon.lock())
					if (auto column = widgets.column.lock()) column->RemoveChild(weak_ptr<Widget>{ oldIcon });
				mIconPaths[index] = data.iconPath;
				if (auto column = widgets.column.lock()) widgets.icon = CreateAbilityIcon(*column, data.iconPath);
				mIcons[index] = widgets.icon;
			}
			if (auto stats = widgets.stats.lock())
			{
				stats->SetString(data.statsText);
				const sf::Vector2f glyphOffset = stats->GetBound().position - stats->GetWidgetLocation();
				UILayout layout = UILayout::Anchored(UIAnchor::BottomLeft, { glyphOffset.x, -94.f + glyphOffset.y });
				layout.pivot = { 0.f, 1.f };
				stats->SetLayout(layout);
			}
			if (auto input = widgets.input.lock())
			{
				input->SetString(data.inputLabel);
				input->SetFillColor(data.accentColor);
				const sf::Vector2f glyphOffset = input->GetBound().position - input->GetWidgetLocation();
				UILayout layout = UILayout::Anchored(UIAnchor::BottomLeft, { 2.f + glyphOffset.x, -88.f + glyphOffset.y });
				layout.pivot = { 0.f, 0.f };
				input->SetLayout(layout);
			}
			if (auto icon = widgets.icon.lock()) icon->SetAlpha(data.state == AbilitySlotState::Cooldown ? 0.35f : data.state == AbilitySlotState::Ready ? 0.9f : 1.f);
			if (auto state = widgets.state.lock())
			{
				if (!data.timing.feedback.empty())
				{
					state->SetString(data.timing.feedback); state->SetFillColor(data.timing.good ? sf::Color{ 100, 255, 150, 255 } : sf::Color{ 255, 125, 100, 255 });
				}
				else if (data.timing.open) { state->SetString("PRESS AGAIN"); state->SetFillColor(sf::Color::White); }
				else if (data.state == AbilitySlotState::Cooldown)
				{
					char buffer[16];
					snprintf(buffer, sizeof(buffer), "%.1fs", static_cast<float>(data.cooldownTenths) / 10.f);
					state->SetString(buffer);
					state->SetFillColor(sf::Color{ 220, 170, 80, 255 });
				}
				else if (data.state == AbilitySlotState::Active)
				{
					char buffer[32];
					if (data.cooldownTenths > 0) snprintf(buffer, sizeof(buffer), "ACTIVE %.1fs", static_cast<float>(data.cooldownTenths) / 10.f);
					else snprintf(buffer, sizeof(buffer), "ACTIVE");
					state->SetString(buffer);
					state->SetFillColor(data.accentColor);
				}
				else
				{
					state->SetString("READY");
					state->SetFillColor(sf::Color{ 120, 255, 140, 255 });
				}
				const sf::Vector2f glyphOffset = state->GetBound().position - state->GetWidgetLocation();
				UILayout layout = UILayout::Anchored(UIAnchor::BottomLeft, { glyphOffset.x, -16.f + glyphOffset.y });
				layout.pivot = { 0.f, 0.f };
				state->SetLayout(layout);
			}
		}
		++mRefreshCount;
	}
}
