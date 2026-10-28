#include "presentation/hud/evolveChoice/EvolveChoicePresenter.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <SFML/Window/Keyboard.hpp>

namespace vega
{
	EvolveChoicePresenter::EvolveChoicePresenter() : mViewModel{ std::make_shared<EvolveChoiceViewModel>() } {}

	void EvolveChoicePresenter::Close()
	{
		mRequestedOpen = false;
		SetIfChanged(mViewModel->open, false, mViewModel->revision);
	}

	void EvolveChoicePresenter::Tick(float deltaTime)
	{
		(void)deltaTime;
		const Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		const auto ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
		const GameAbility* pendingAbility = nullptr;
		if (ship && !ship->GetIsPendingDestroy() && player->GetPendingSecondaryWeaponChoices().empty())
		{
			const auto& abilities = ship->GetAbilitySystemComponent();
			for (bool second : { false, true })
			{
				for (auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
				{
					const auto* ability = second ? abilities.GetSecondAbility(slot) : abilities.GetAbility(slot);
					if (ability && ability->GetPendingEvolve().tierLevel) { pendingAbility = ability; break; }
				}
				if (pendingAbility) break;
			}
		}
		if (!pendingAbility) mRequestedOpen = false;
		const auto pending = pendingAbility ? pendingAbility->GetPendingEvolve() : PendingAbilityEvolve{};
		SetIfChanged(mViewModel->open, mRequestedOpen && pendingAbility != nullptr, mViewModel->revision);
		SetIfChanged(mViewModel->abilityId, pendingAbility ? pendingAbility->GetDefinition().abilityId : std::string{}, mViewModel->revision);
		SetIfChanged(mViewModel->abilityName, pendingAbility ? pendingAbility->GetDefinition().displayName : std::string{}, mViewModel->revision);
		SetIfChanged(mViewModel->tierLevel, pending.tierLevel, mViewModel->revision);
		SetIfChanged(mViewModel->options, pending.options, mViewModel->revision);
	}

	bool EvolveChoicePresenter::HandleEvent(const sf::Event& event)
	{
		if (const auto* released = event.getIf<sf::Event::KeyReleased>())
		{
			if (released->code == sf::Keyboard::Key::V) mToggleKeyDown = false;
			return false;
		}
		const auto* pressed = event.getIf<sf::Event::KeyPressed>();
		if (!pressed) return false;
		Tick(0.f);
		if (pressed->code == sf::Keyboard::Key::V)
		{
			if (mViewModel->options.empty()) return false;
			if (!mToggleKeyDown) { mRequestedOpen = !mRequestedOpen; Tick(0.f); }
			mToggleKeyDown = true;
			return true;
		}
		if (!mViewModel->open) return false;
		if (pressed->code == sf::Keyboard::Key::Escape) { Close(); return true; }
		if (pressed->code == sf::Keyboard::Key::Tab) { Close(); return false; }
		const int index = static_cast<int>(pressed->code) - static_cast<int>(sf::Keyboard::Key::Num1);
		if (index < 0 || index > 2) return false;
		(void)Choose(static_cast<std::size_t>(index));
		return true;
	}

	bool EvolveChoicePresenter::Choose(std::size_t index)
	{
		Tick(0.f);
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player || !mViewModel->open || index >= mViewModel->options.size()) return false;
		const bool chosen = player->TryChooseAbilityEvolve(mViewModel->abilityId, mViewModel->options[index].id);
		Tick(0.f);
		return chosen;
	}
}
