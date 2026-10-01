#include "presentation/hud/ability/AbilityBarPresenter.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace ly
{
	namespace
	{
		constexpr std::size_t kMaxStatLines = 6;

		std::string ShortAbilityName(const std::string& id)
		{
			std::string trimmed = id;
			const std::size_t basic = trimmed.rfind(".Basic");
			if (basic != std::string::npos && basic + 6 == trimmed.size()) trimmed.erase(basic);
			const std::size_t dot = trimmed.rfind('.');
			return dot == std::string::npos ? trimmed : trimmed.substr(dot + 1);
		}

		std::string FormatStatValue(float value)
		{
			char buffer[32];
			if (std::fabs(value - std::round(value)) < 0.005f)
				snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(std::lround(value)));
			else
				snprintf(buffer, sizeof(buffer), "%.2f", value);
			return buffer;
		}

		// TEMPORARY TEST UI: per-slot level/stat readout, rebuilt only when an ability changes.
		std::string BuildStatsText(LightYearsAbilitySystemComponent& abilitySystem, GameAbility& ability)
		{
			const GameAbilityDefinition& definition = ability.GetDefinition();
			std::string text = ShortAbilityName(definition.abilityId) + "\nLv " + std::to_string(ability.GetLevel());
			const float cooldown = ability.GetCooldownDuration();
			if (cooldown > 0.f) text += "  CD " + FormatStatValue(cooldown) + "s";

			AbilityExecutionContext context;
			context.abilitySystem = &abilitySystem;
			context.definition = &definition;
			context.instance = &ability;
			const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAbilityAttributes(context);
			std::size_t shown = 0;
			for (const sas::GameplayAttribute& value : values)
			{
				if (shown++ >= kMaxStatLines) break;
				const std::string_view name = value.id.GetName();
				const std::size_t dot = name.rfind('.');
				text += "\n" + std::string{ dot == std::string_view::npos ? name : name.substr(dot + 1) } +
					" " + FormatStatValue(value.currentValue);
			}
			return text;
		}
	}

	AbilityBarPresenter::AbilityBarPresenter() : mViewModel{ std::make_shared<AbilityBarViewModel>() }
	{
	}

	void AbilityBarPresenter::BindShip(const shared_ptr<PlayerSpaceShip>& ship)
	{
		mShipSubs.Clear();
		mBoundShip = ship;
		mBoundShipId = ship->GetUniqueID();
		mHasBoundShip = true;
		mStatsDirty = true;

		LightYearsAbilitySystemComponent& abilitySystem = ship->GetAbilitySystemComponent();
		mShipSubs.Bind(ship, abilitySystem.onAbilityGranted, this, &AbilityBarPresenter::OnAbilityChanged);
		mShipSubs.Bind(ship, abilitySystem.onAbilityRemoved, this, &AbilityBarPresenter::OnAbilityChanged);
		mShipSubs.Bind(ship, abilitySystem.onAbilityChanged, this, &AbilityBarPresenter::OnAbilityChanged);
		mShipSubs.Bind(ship, abilitySystem.onAbilityLevelChanged, this, &AbilityBarPresenter::OnAbilityLevelChanged);
		SetIfChanged(mViewModel->shipGeneration, mViewModel->shipGeneration + 1, mViewModel->revision);
	}

	void AbilityBarPresenter::ClearShip()
	{
		mShipSubs.Clear();
		mBoundShip.reset();
		mBoundShipId = 0;
		mHasBoundShip = false;
		mStatsDirty = true;
		for (AbilitySlotViewData& slot : mViewModel->slots)
			SetIfChanged(slot, AbilitySlotViewData{}, mViewModel->revision);
	}

	void AbilityBarPresenter::OnAbilityChanged(sas::AbilityHandle)
	{
		mStatsDirty = true;
	}

	void AbilityBarPresenter::OnAbilityLevelChanged(sas::AbilityHandle, int)
	{
		mStatsDirty = true;
	}

	void AbilityBarPresenter::RefreshStats()
	{
		if (!mStatsDirty) return;
		const shared_ptr<PlayerSpaceShip> ship = mBoundShip.lock();
		if (!ship || ship->GetIsPendingDestroy()) return;
		mStatsDirty = false;
		LightYearsAbilitySystemComponent& abilitySystem = ship->GetAbilitySystemComponent();
		for (std::size_t index = 0; index < mViewModel->slots.size(); ++index)
		{
			AbilitySlotViewData projected = mViewModel->slots[index];
			const sas::AbilitySlot slot = static_cast<sas::AbilitySlot>(
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index));
			GameAbility* ability = abilitySystem.GetAbility(slot);
			projected.statsText = ability && projected.visible ? BuildStatsText(abilitySystem, *ability) : std::string{};
			SetIfChanged(mViewModel->slots[index], projected, mViewModel->revision);
		}
	}

	void AbilityBarPresenter::Tick()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		const shared_ptr<PlayerSpaceShip> ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
		if (!ship || ship->GetIsPendingDestroy())
		{
			if (mHasBoundShip || std::any_of(mViewModel->slots.begin(), mViewModel->slots.end(),
				[](const AbilitySlotViewData& slot) { return slot.visible; })) ClearShip();
			return;
		}

		if (!mHasBoundShip || mBoundShipId != ship->GetUniqueID()) BindShip(ship);
		LightYearsAbilitySystemComponent& abilitySystem = ship->GetAbilitySystemComponent();
		for (std::size_t index = 0; index < mViewModel->slots.size(); ++index)
		{
			const sas::AbilitySlot slot = static_cast<sas::AbilitySlot>(
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index));
			GameAbility* ability = abilitySystem.GetAbility(slot);
			AbilitySlotViewData projected{};
			projected.statsText = mViewModel->slots[index].statsText;
			if (ability && !ability->GetDefinition().iconPath.empty())
			{
				const GameAbilityDefinition& definition = ability->GetDefinition();
				projected.visible = true;
				projected.iconPath = definition.iconPath;
				projected.inputLabel = AbilityInputSchema::GetLabel(slot);
				projected.accentColor = definition.accentColor;
				if (ability->IsActive()) projected.state = AbilitySlotState::Active;
				else if (ability->IsOnCooldown())
				{
					projected.state = AbilitySlotState::Cooldown;
					projected.cooldownTenths = static_cast<int>(std::lround(ability->GetCooldownRemaining() * 10.f));
				}
			}
			SetIfChanged(mViewModel->slots[index], projected, mViewModel->revision);
		}
		RefreshStats();
	}
}
