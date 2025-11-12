#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include <framework/World.h>

namespace ly
{
	Player::Player():
		mLifeCount{3},
		mScore{ 0 },
		mCurrentSpaceShip{}
	{
	}
	weak_ptr<PlayerSpaceShip> Player::SpawnSpaceShip(World* world)
	{
		if (mLifeCount > 0)
		{
			--mLifeCount;
			auto windowSize = world->GetWindowSize();
			mCurrentSpaceShip = world->SpawnActor<PlayerSpaceShip>();
			mCurrentSpaceShip.lock()->SetActorLocation(sf::Vector2f{ windowSize.x / 2.0f, windowSize.y - 100.0f });

			onLifeChange.Broadcast(mLifeCount);
			return mCurrentSpaceShip;
		}

		else 
		{
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
}