#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include <framework/World.h>

namespace ly
{
	Player::Player():
		mLifeCount{3},
		mScore{ 0 },
		mScrap{ 0 },
		mCurrentSpaceShip{}
	{
	}
	
	weak_ptr<PlayerSpaceShip> Player::SpawnSpaceShip(World* world) {
		if (mLifeCount > 0)
		{
			--mLifeCount;
			mShipProgression.UnbindAttributes();
			auto windowSize = world->GetWindowSize();
			mCurrentSpaceShip = world->SpawnActor<PlayerSpaceShip>();

			auto ship = mCurrentSpaceShip.lock();
			if (!ship)
			{
				mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
				onLifeExhausted.Broadcast();
				return weak_ptr<PlayerSpaceShip>{};
			}

			if (!mShipProgression.IsConfigured())
			{
				mShipProgression.Configure(ShipData::Ship_Player_Fighter.progressionDefinition);
			}
			mShipProgression.BindAttributes(ship->GetAbilitySystemComponent().GetAttributes());
			RestorePurchasedAbilityLevels(*ship);
			ship->onActorDestroyed.BindAction(this, &Player::OnCurrentShipDestroyed);

			ship->SetActorLocation(sf::Vector2f{ windowSize.x / 2.0f, windowSize.y - 100.0f });

			onLifeChange.Broadcast(mLifeCount);
			return mCurrentSpaceShip;
		}
		else
		{
			mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
			ResetRunProgression();
			onLifeExhausted.Broadcast();
			return weak_ptr<PlayerSpaceShip>{};
		}
	}

	void Player::AddLifeCount(unsigned int count)
	{
		if(count<=0)
			return;
		mLifeCount += count;
		onLifeChange.Broadcast(mLifeCount);
	}

	void Player::AddScore(unsigned int amt)
	{
		if (amt <= 0)
			return;
		mScore += amt;
		onScoreChange.Broadcast(mScore);
	}

	void Player::AwardShipXP(float amount)
	{
		mShipProgression.AddXP(amount);
	}

	void Player::AddShipXP(float amount)
	{
		AwardShipXP(amount);
	}

	void Player::AwardScrap(unsigned int amount)
	{
		if (amount == 0)
		{
			return;
		}

		mScrap += amount;
		onScrapChange.Broadcast(mScrap);
	}

	bool Player::TryPurchaseAbilityLevel(sas::AbilitySlot slot, std::string* failureReason)
	{
		const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
		if (!ship || ship->GetIsPendingDestroy())
		{
			if (failureReason)
			{
				*failureReason = "No active ship is available for an upgrade.";
			}
			return false;
		}

		sas::AbilitySystemComponent& abilities =
			ship->GetAbilitySystemComponent();
		GameAbility* ability =
			abilities.FindAbility<GameAbility>(slot);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "No ability is assigned to this slot.";
			}
			return false;
		}

		const int targetLevel = ability->GetLevel() + 1;
		if (targetLevel > ability->GetMaxLevel())
		{
			if (failureReason)
			{
				*failureReason = "Ability is already at maximum level.";
			}
			return false;
		}

		const GameAbilityDefinition& definition = ability->GetDefinition();
		if (!definition.HasScrapCostToReachLevel(targetLevel))
		{
			if (failureReason)
			{
				*failureReason = "This ability has no configured scrap upgrade cost.";
			}
			return false;
		}

		const unsigned int cost = definition.GetScrapCostToReachLevel(targetLevel);
		if (mScrap < cost)
		{
			if (failureReason)
			{
				*failureReason = "Not enough scrap for this upgrade.";
			}
			return false;
		}

		const std::string abilityId = definition.abilityId;
		if (!abilities.LevelUpAbility(slot))
		{
			if (failureReason)
			{
				*failureReason = "Ability level could not be increased.";
			}
			return false;
		}

		mScrap -= cost;
		mPurchasedAbilityLevels[abilityId] = ability->GetLevel();
		onScrapChange.Broadcast(mScrap);
		return true;
	}
	
	void Player::OnScoreAwarded(unsigned int scoreAmount)
	{
		AddScore(scoreAmount);
	}

	void Player::RestorePurchasedAbilityLevels(PlayerSpaceShip& ship)
	{
		sas::AbilitySystemComponent& abilities =
			ship.GetAbilitySystemComponent();
		for (auto& purchasedLevel : mPurchasedAbilityLevels)
		{
			GameAbility* ability =
				abilities.FindAbilityById<GameAbility>(
					purchasedLevel.first
				);
			if (!ability)
			{
				continue;
			}

			abilities.SetAbilityLevel(
				ability->GetHandle(),
				purchasedLevel.second
			);
			purchasedLevel.second = ability->GetLevel();
		}
	}

	void Player::OnCurrentShipDestroyed(Actor* destroyedActor)
	{
		const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock();
		if (!currentShip || currentShip.get() != destroyedActor)
		{
			return;
		}

		mShipProgression.UnbindAttributes();
		mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
	}

	void Player::ResetRunProgression()
	{
		mShipProgression.ResetForNewRun();
		mPurchasedAbilityLevels.clear();
		if (mScrap != 0)
		{
			mScrap = 0;
			onScrapChange.Broadcast(mScrap);
		}
	}

}


