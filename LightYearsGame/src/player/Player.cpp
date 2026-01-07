#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include <framework/World.h>

namespace ly
{
	Player::Player():
		mLifeCount{3},
		mScore{ 0 },
		mCurrentSpaceShip{},
		mSavedWeaponState{}
	{
	}
	
	weak_ptr<PlayerSpaceShip> Player::SpawnSpaceShip(World* world) {
		if (mLifeCount > 0)
		{
			--mLifeCount;
			auto windowSize = world->GetWindowSize();
			mCurrentSpaceShip = world->SpawnActor<PlayerSpaceShip>();

			auto ship = mCurrentSpaceShip.lock();
			if (!ship)
			{
				mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
				onLifeExhausted.Broadcast();
				return weak_ptr<PlayerSpaceShip>{};
			}

			ship->SetActorLocation(sf::Vector2f{ windowSize.x / 2.0f, windowSize.y - 100.0f });
			ship->onWeaponStateBeforeDeath.BindAction(this, &Player::OnWeaponStateReceived);

			WeaponState degradedState = GetDegradedWeaponState();;
			ship->ApplyWeaponState(degradedState);
			ClearWeaponState();

			onLifeChange.Broadcast(mLifeCount);
			return mCurrentSpaceShip;
		}
		else
		{
			mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
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
	
	void Player::OnScoreAwarded(unsigned int scoreAmount)
	{
		LOG("===== PLAYER::OnScoreAwarded CALLED =====");
		LOG("Score Amount Received: %u", scoreAmount);
		AddScore(scoreAmount);
	}

	void Player::OnWeaponStateReceived(WeaponType type, int level)
	{
		mSavedWeaponState.type = type;
		mSavedWeaponState.level = level;
	}

	WeaponState Player::GetDegradedWeaponState() const
	{
		WeaponState degraded;

		if (mSavedWeaponState.type == WeaponType::Default || mSavedWeaponState.level <= 0)
		{
			degraded.type = WeaponType::Default;
			degraded.level = 1;
			LOG("No saved weapon or default weapon, returning Default level 1");
			return degraded;
		}

		int newLevel = mSavedWeaponState.level - 2;

		if (newLevel < 1)
		{
			degraded.type = WeaponType::Default;
			degraded.level = 1;
			LOG("Weapon degraded to Default (newLevel would be %d)", newLevel);
		}
		else
		{
			degraded.type = mSavedWeaponState.type;
			degraded.level = newLevel;
			LOG("Weapon degraded: Type=%d, Level=%d -> %d",
				(int)degraded.type, mSavedWeaponState.level, newLevel);
		}

		return degraded;
	}

	void Player::ClearWeaponState()
	{
		mSavedWeaponState = WeaponState{};
	}
}