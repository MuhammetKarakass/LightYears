#include "presentation/hud/vitals/VitalsPresenter.h"
#include "gameConfigs/combat/EffectStructs.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/HealthComponent.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <algorithm>
#include <cstdint>

namespace ly
{
	sf::Color ComputeHealthBarColor(const VitalsViewModel& vm)
	{
		if (vm.healthMax <= 0.f)
			return sf::Color{ 255, 0, 0, 255 };
		if (vm.displayHealthMax > vm.healthMax)
			return sf::Color{ 80, 160, 255, 255 };

		const float healthPercent = vm.health / vm.healthMax;
		std::uint8_t r, g;
		if (healthPercent >= 0.5f)
		{
			const float t = (healthPercent - 0.5f) * 2.0f;
			r = static_cast<std::uint8_t>(255 * (1.0f - t));
			g = 255;
		}
		else
		{
			const float t = healthPercent * 2.0f;
			r = 255;
			g = static_cast<std::uint8_t>(255 * t);
		}
		return sf::Color{ r, g, 0, 255 };
	}

	VitalsPresenter::VitalsPresenter() : mViewModel{ std::make_shared<VitalsViewModel>() }
	{
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		mManagerSubs.BindUnguarded(manager.onPlayerCreated, this, &VitalsPresenter::OnPlayerCreated);
		mManagerSubs.BindUnguarded(manager.onPlayerAboutToBeDestroyed, this, &VitalsPresenter::OnPlayerAboutToBeDestroyed);
		BindPlayer(manager.GetPlayer());
	}

	void VitalsPresenter::OnPlayerCreated(Player* player)
	{
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		if (player == manager.GetPlayer())
		{
			mHasPlayerBeingDestroyed = false;
			BindPlayer(player);
		}
	}

	void VitalsPresenter::OnPlayerAboutToBeDestroyed(Player* player)
	{
		if (!player || !mHasBoundPlayer || player->GetUniqueID() != mBoundPlayerId)
			return;

		mPlayerBeingDestroyedId = mBoundPlayerId;
		mHasPlayerBeingDestroyed = true;
		mPlayerSubs.Clear();
		mBoundPlayerId = 0;
		mHasBoundPlayer = false;
		SetIfChanged(mViewModel->hasPlayer, false, mViewModel->revision);
		SetIfChanged(mViewModel->life, 0u, mViewModel->revision);
		SetIfChanged(mViewModel->score, 0u, mViewModel->revision);
		ClearShipValues();
	}

	void VitalsPresenter::OnLifeChanged(int life)
	{
		SetIfChanged(mViewModel->life, static_cast<unsigned int>(life), mViewModel->revision);
	}

	void VitalsPresenter::OnScoreChanged(int score)
	{
		SetIfChanged(mViewModel->score, static_cast<unsigned int>(score), mViewModel->revision);
	}

	void VitalsPresenter::BindPlayer(Player* player)
	{
		if (!player || (mHasPlayerBeingDestroyed && player->GetUniqueID() == mPlayerBeingDestroyedId) || (mHasBoundPlayer && player->GetUniqueID() == mBoundPlayerId))
			return;

		mPlayerSubs.Clear();
		mBoundPlayerId = player->GetUniqueID();
		mHasBoundPlayer = true;
		mHasPlayerBeingDestroyed = false;
		mPlayerSubs.BindUnguarded(player->onLifeChange, this, &VitalsPresenter::OnLifeChanged);
		mPlayerSubs.BindUnguarded(player->onScoreChange, this, &VitalsPresenter::OnScoreChanged);
		SetIfChanged(mViewModel->hasPlayer, true, mViewModel->revision);
		SetIfChanged(mViewModel->life, player->GetLifeCount(), mViewModel->revision);
		SetIfChanged(mViewModel->score, player->GetScore(), mViewModel->revision);
	}

	void VitalsPresenter::ClearShipValues()
	{
		SetIfChanged(mViewModel->hasShip, false, mViewModel->revision);
		SetIfChanged(mViewModel->health, 0.f, mViewModel->revision);
		SetIfChanged(mViewModel->healthMax, 1.f, mViewModel->revision);
		SetIfChanged(mViewModel->displayHealth, 0.f, mViewModel->revision);
		SetIfChanged(mViewModel->displayHealthMax, 1.f, mViewModel->revision);
		SetIfChanged(mViewModel->shield, 0.f, mViewModel->revision);
		SetIfChanged(mViewModel->shieldMax, 1.f, mViewModel->revision);
		SetIfChanged(mViewModel->energy, 0.f, mViewModel->revision);
		SetIfChanged(mViewModel->energyMax, 1.f, mViewModel->revision);
	}

	void VitalsPresenter::Tick()
	{
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		Player* player = manager.GetPlayer();
		if (!player)
		{
			if (mHasBoundPlayer)
			{
				mPlayerSubs.Clear();
				mBoundPlayerId = 0;
				mHasBoundPlayer = false;
				SetIfChanged(mViewModel->hasPlayer, false, mViewModel->revision);
				SetIfChanged(mViewModel->life, 0u, mViewModel->revision);
				SetIfChanged(mViewModel->score, 0u, mViewModel->revision);
			}
			mHasPlayerBeingDestroyed = false;
			ClearShipValues();
			return;
		}

		const unsigned int currentPlayerId = player->GetUniqueID();
		if (mHasPlayerBeingDestroyed && currentPlayerId == mPlayerBeingDestroyedId)
		{
			ClearShipValues();
			return;
		}
		mHasPlayerBeingDestroyed = false;
		if (!mHasBoundPlayer || currentPlayerId != mBoundPlayerId)
			BindPlayer(player);

		shared_ptr<PlayerSpaceShip> ship = player->GetCurrentSpaceShip().lock();
		if (!ship || ship->GetIsPendingDestroy())
		{
			ClearShipValues();
			return;
		}

		VitalsViewModel& vm = *mViewModel;
		const HealthComponent& health = ship->GetHealthComponent();
		const float currentHealth = health.GetHealth();
		const float maxHealth = health.GetMaxHealth();
		float displayHealth = currentHealth;
		float displayHealthMax = maxHealth;
		if (maxHealth <= 0.f)
		{
			displayHealth = 0.f;
			displayHealthMax = 1.f;
		}
		else
		{
			for (const sas::GameplayEffectRuntimeSnapshot& effect : ship->GetAbilitySystemComponent().BuildGameplayEffectSnapshots())
			{
				const sas::GameplayAttribute* capacity = sas::FindAttribute(effect.runtimeAttributes, BarrierEffectSchema::Capacity);
				if (capacity && capacity->baseValue > 0.f)
				{
					displayHealth += capacity->currentValue;
					displayHealthMax += capacity->baseValue;
				}
			}
		}

		const ShieldComponent& shield = ship->GetShieldComponent();
		const EnergyComponent& energy = ship->GetEnergyComponent();
		SetIfChanged(vm.hasShip, true, vm.revision);
		SetIfChanged(vm.health, currentHealth, vm.revision);
		SetIfChanged(vm.healthMax, maxHealth, vm.revision);
		SetIfChanged(vm.displayHealth, displayHealth, vm.revision);
		SetIfChanged(vm.displayHealthMax, displayHealthMax, vm.revision);
		SetIfChanged(vm.shield, shield.GetMaxShield() > 0.f ? std::max(0.f, shield.GetShield()) : 0.f, vm.revision);
		SetIfChanged(vm.shieldMax, shield.GetMaxShield() > 0.f ? shield.GetMaxShield() : 1.f, vm.revision);
		SetIfChanged(vm.energy, std::max(0.f, energy.GetEnergy()), vm.revision);
		SetIfChanged(vm.energyMax, energy.GetMaxEnergy() > 0.f ? energy.GetMaxEnergy() : 1.f, vm.revision);
	}
}
