#include "presentation/hud/ability/AbilityBarPresenter.h"

#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace vega
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

	std::string BuildPrimaryWeaponReadout(PlayerSpaceShip& ship)
	{
		auto& abilities = ship.GetAbilitySystemComponent();
		const auto* hardpoint = abilities.GetSecondHardpointRuntime();
		const auto* meltdown = abilities.GetMeltdownBarrelRuntime();
		if (!hardpoint && !meltdown) return {};
		auto* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		if (!primary) return {};
		const PrimaryWeaponDefinition* weapon = nullptr;
		const PrimaryWeaponRuntimeState* state = &primary->GetPrimaryWeaponRuntime();
		for (const auto& action : primary->GetDefinition().actions)
			if (const auto* fire = std::get_if<FireWeaponAction>(&action.action)) { weapon = &fire->weaponDefinition; break; }
		if (hardpoint && hardpoint->IsSecondWeaponActive())
		{
			weapon = hardpoint->GetSecondWeapon();
			state = &abilities.GetSecondHardpointRuntime()->GetSecondWeaponState().runtime;
		}
		if (const auto* form = abilities.GetActivePrimaryWeaponOverride()) { weapon = &form->weaponDefinition; state = &form->runtime; }
		if (!weapon) return {};
		std::string text = ShortAbilityName(weapon->weaponId);
		if (hardpoint && hardpoint->GetSecondWeapon()) text += hardpoint->IsSecondWeaponActive() ? "  [2/2]" : "  [1/2]";
		if (meltdown && meltdown->Supports(*weapon))
		{
			const float heat = weapon->magazine ? (state->meltdownCycle && state->meltdownCycle->active ? state->meltdownCycle->heat : 0.f) : state->GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue);
			text += "\nHeat " + FormatStatValue(heat);
		}
		else if (weapon->magazine) text += "\nAmmo " + std::to_string(state->isInitialized ? state->magazineState.roundsRemaining : weapon->magazine->capacity) + "/" + std::to_string(weapon->magazine->capacity);
		if (hardpoint && hardpoint->IsSwitching()) text += "\nSwitch " + FormatStatValue(hardpoint->GetSwitchRemaining()) + "s";
		if (state->meltdownCycle && state->meltdownCycle->active && state->meltdownCycle->focusRemaining > 0.f) text += "\nNova " + FormatStatValue(state->meltdownCycle->focusRemaining) + "s";
		if (hardpoint && hardpoint->GetSecondWeapon()) text += "\nMouse wheel: switch";
		return text;
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
		SetIfChanged(mViewModel->weaponReadout, std::string{}, mViewModel->revision);
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
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index % 4));
			GameAbility* ability = index < 4 ? abilitySystem.GetAbility(slot) : abilitySystem.GetSecondAbility(slot);
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
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index % 4));
			GameAbility* ability = index < 4 ? abilitySystem.GetAbility(slot) : abilitySystem.GetSecondAbility(slot);
			AbilitySlotViewData projected{};
			projected.statsText = mViewModel->slots[index].statsText;
			if (ability && (index < 4 || abilitySystem.IsTwinRackEnabled()) && !ability->GetDefinition().iconPath.empty())
			{
				const GameAbilityDefinition& definition = ability->GetDefinition();
				projected.visible = true;
				projected.iconPath = definition.iconPath;
				projected.inputLabel = (index < 4 ? "" : "Ctrl+") + std::string{ AbilityInputSchema::GetLabel(slot) };
				projected.accentColor = definition.accentColor;
				if (abilitySystem.IsTwinRackEnabled() && ability->IsOnCooldown()) projected.cooldownTenths = static_cast<int>(std::lround(ability->GetCooldownRemaining() * 10.f));
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
		SetIfChanged(mViewModel->weaponReadout, BuildPrimaryWeaponReadout(*ship), mViewModel->revision);
	}
}
