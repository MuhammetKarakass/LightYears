#include "spaceShip/SpaceShip.h"
#include <framework/World.h>

namespace ly
{

	SpaceShip::SpaceShip(World* owningWorld, std::string texturePath):
		Actor(owningWorld, texturePath),
		mVelocity(0.0f, 0.0f),
		mHealthComponent{100.f,100.f}
	{
	}

	void SpaceShip::BeginPlay()
	{
		Actor::BeginPlay();
		SetEnablePhysics(true);
		mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
		mHealthComponent.onHealthChanged.Broadcast(11, 89, 100);
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

	void SpaceShip::Shoot()
	{

	}
	void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
	{
		LOG("amt:%f health percentage %f/%f", amt, health, maxHealth);
	}
}