#include "spaceShip/SpaceShip.h"
#include <framework/World.h>

namespace ly
{
	SpaceShip::SpaceShip(World* owningWorld, std::string texturePath):
		Actor(owningWorld, texturePath),
		mVelocity(0.0f, 0.0f)
	{
	}
	void SpaceShip::Tick(float deltaTime)
	{
        Actor::Tick(deltaTime);
		AddActorLocationOffset(mVelocity * deltaTime);
	}

	void SpaceShip::SetVelocity(const sf::Vector2f& velocity)
	{
		mVelocity = velocity;
	}
}