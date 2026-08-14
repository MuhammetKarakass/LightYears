#include "presentation/hud/ability/AbilityUIController.h"
#include "widget/GameHUD.h"
#include "player/PlayerManager.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/input/AbilityInputSchema.h"
#include <cstdio>

namespace ly
{
	AbilityUIController::AbilityUIController(weak_ptr<GameHUD> gameHUD)
		: mGameHUD{ gameHUD },
		mViewModel{},
		mSlotWidgets{},
		mWidgetsCreated{ false },
		mObservedShipId{ 0 },
		mLastWindowSize{ 0, 0 }
	{
	}

	void AbilityUIController::Tick(float deltaTime)
	{
		(void)deltaTime;

		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit())
		{
			return;
		}

		EnsureWidgetsCreated();
		RefreshFromAbilitySystem();

		const sf::Vector2u windowSize = hud->GetWindowSize();
		if (windowSize != mLastWindowSize)
		{
			mLastWindowSize = windowSize;
			PositionWidgets();
		}
	}

	void AbilityUIController::EnsureWidgetsCreated()
	{
		auto hud = mGameHUD.lock();
		if (!hud)
		{
			return;
		}

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player || player->GetCurrentSpaceShip().expired())
		{
			if (mWidgetsCreated)
			{
				ResetWidgets();
			}
			return;
		}

		auto ship = player->GetCurrentSpaceShip().lock();
		if (!ship || ship->GetIsPendingDestroy())
		{
			if (mWidgetsCreated)
			{
				ResetWidgets();
			}
			return;
		}

		if (mWidgetsCreated && mObservedShipId != ship->GetUniqueID())
		{
			ResetWidgets();
		}

		const sas::AbilitySystemComponent& abilitySystem =
			ship->GetAbilitySystemComponent();
		bool widgetsChanged = false;
		for (auto it = mSlotWidgets.begin(); it != mSlotWidgets.end();)
		{
			if (abilitySystem.FindAbility<GameAbility>(it->first))
			{
				++it;
				continue;
			}

			hud->RemoveWidget(it->second.icon);
			hud->RemoveWidget(it->second.inputLabel);
			hud->RemoveWidget(it->second.stateText);
			mViewModel.SetAvailable(it->first, false);
			it = mSlotWidgets.erase(it);
			widgetsChanged = true;
		}

		static const sas::AbilitySlot abilitySlots[] = {
			sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4
		};

		for (sas::AbilitySlot slot : abilitySlots)
		{
			if (mSlotWidgets.find(slot) != mSlotWidgets.end())
			{
				continue;
			}

			const GameAbility* ability =
				abilitySystem.FindAbility<GameAbility>(slot);
			if (!ability)
			{
				continue;
			}

			const GameAbilityDefinition& definition = ability->GetDefinition();
			if (definition.iconPath.empty())
			{
				continue;
			}

			mViewModel.RegisterSlot(slot, definition);

			auto icon = hud->AddWidget<ImageWidget>(definition.iconPath);
			if (auto iconLocked = icon.lock())
			{
				iconLocked->SetVisibility(true);
			}

			auto inputLabel = hud->AddWidget<TextWidget>(
				AbilityInputSchema::GetLabel(slot),
				"SpaceShooterRedux/Bonus/OrbitronBlack.ttf"
			);
			if (auto labelLocked = inputLabel.lock())
			{
				labelLocked->SetTextSize(14);
				labelLocked->SetFillColor(definition.accentColor);
				labelLocked->SetVisibility(true);
			}

			auto stateText = hud->AddWidget<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf");
			if (auto textLocked = stateText.lock())
			{
				textLocked->SetTextSize(15);
				textLocked->SetFillColor(sf::Color::White);
				textLocked->SetVisibility(true);
			}

			mSlotWidgets[slot] = SlotWidgets{ icon, inputLabel, stateText };
			widgetsChanged = true;
		}

		mLastWindowSize = hud->GetWindowSize();
		mWidgetsCreated = true;
		mObservedShipId = ship->GetUniqueID();
		if (widgetsChanged)
		{
			PositionWidgets();
		}
	}

	void AbilityUIController::ResetWidgets()
	{
		auto hud = mGameHUD.lock();
		if (hud)
		{
			for (const auto& [slot, widgets] : mSlotWidgets)
			{
				(void)slot;
				hud->RemoveWidget(widgets.icon);
				hud->RemoveWidget(widgets.inputLabel);
				hud->RemoveWidget(widgets.stateText);
			}
		}

		mSlotWidgets.clear();
		mViewModel = AbilityUIViewModel{};
		mWidgetsCreated = false;
		mObservedShipId = 0;
	}

	void AbilityUIController::RefreshFromAbilitySystem()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player || player->GetCurrentSpaceShip().expired())
		{
			return;
		}

		auto ship = player->GetCurrentSpaceShip().lock();
		if (!ship || ship->GetIsPendingDestroy())
		{
			return;
		}

		const sas::AbilitySystemComponent& abilitySystem =
			ship->GetAbilitySystemComponent();

		for (auto& [slot, widgets] : mSlotWidgets)
		{
			(void)widgets;

			const GameAbility* ability =
				abilitySystem.FindAbility<GameAbility>(slot);
			if (!ability)
			{
				mViewModel.SetAvailable(slot, false);
				continue;
			}

			const GameAbilityDefinition& definition = ability->GetDefinition();
			mViewModel.RegisterSlot(slot, definition);

			if (ability->IsOnCooldown())
			{
				mViewModel.UpdateCooldown(slot, ability->GetCooldownRemaining(), ability->GetCooldownDuration());
			}
			else
			{
				mViewModel.UpdateCooldown(slot, 0.f, 1.f);
			}

			if (ability->IsActive())
			{
				mViewModel.UpdateActive(slot, true, ability->GetActiveTimeRemaining(), ability->GetActiveDuration());
			}
			else
			{
				mViewModel.UpdateActive(slot, false, 0.f, 1.f);
			}

			UpdateWidgetVisuals(slot);
		}
	}

	void AbilityUIController::UpdateWidgetVisuals(sas::AbilitySlot slot)
	{
		const AbilitySlotUIData* data = mViewModel.GetSlotData(slot);
		if (!data || !data->isAvailable)
		{
			return;
		}

		auto found = mSlotWidgets.find(slot);
		if (found == mSlotWidgets.end())
		{
			return;
		}

		const SlotWidgets& widgets = found->second;

		if (auto icon = widgets.icon.lock())
		{
			if (data->isActive)
			{
				icon->SetAlpha(1.f);
			}
			else if (data->cooldownPercent > 0.f)
			{
				icon->SetAlpha(0.35f);
			}
			else
			{
				icon->SetAlpha(0.9f);
			}
		}

		if (auto inputLabel = widgets.inputLabel.lock())
		{
			inputLabel->SetString(data->inputLabel);
			inputLabel->SetFillColor(data->accentColor);
			inputLabel->SetVisibility(true);
		}

		if (auto text = widgets.stateText.lock())
		{
			if (data->cooldownPercent > 0.f && !data->isActive)
			{
				char buffer[16];
				snprintf(buffer, sizeof(buffer), "%.1fs", data->cooldownRemaining);
				text->SetString(buffer);
				text->SetFillColor(sf::Color{ 220, 170, 80, 255 });
				text->SetVisibility(true);
			}
			else if (data->isActive)
			{
				text->SetString("ACTIVE");
				text->SetFillColor(data->accentColor);
				text->SetVisibility(true);
			}
			else
			{
				text->SetString("READY");
				text->SetFillColor(sf::Color{ 120, 255, 140, 255 });
				text->SetVisibility(true);
			}
		}
	}

	void AbilityUIController::PositionWidgets()
	{
		const sf::Vector2u windowSize = mLastWindowSize;
		if (windowSize.x == 0 || windowSize.y == 0)
		{
			return;
		}

		const float iconSize = 48.f;
		const float spacing = 12.f;

		int slotCount = 0;
		for (const auto& [slot, widgets] : mSlotWidgets)
		{
			(void)slot;
			if (!widgets.icon.expired())
			{
				++slotCount;
			}
		}

		if (slotCount == 0)
		{
			return;
		}

		const float totalWidth = static_cast<float>(slotCount) * iconSize + static_cast<float>(slotCount - 1) * spacing;
		const float startX = static_cast<float>(windowSize.x) * 0.5f - totalWidth * 0.5f;
		const float baseY = static_cast<float>(windowSize.y) - 112.f;

		int index = 0;
		for (const auto& [slot, widgets] : mSlotWidgets)
		{
			(void)slot;
			const float x = startX + static_cast<float>(index) * (iconSize + spacing);

			if (auto icon = widgets.icon.lock())
			{
				icon->SetWidgetLocation(sf::Vector2f{ x, baseY });
			}

			if (auto inputLabel = widgets.inputLabel.lock())
			{
				inputLabel->SetWidgetLocation(sf::Vector2f{ x + 2.f, baseY - 20.f });
			}

			if (auto stateText = widgets.stateText.lock())
			{
				stateText->SetWidgetLocation(sf::Vector2f{ x, baseY + iconSize + 4.f });
			}

			++index;
		}
	}
}


