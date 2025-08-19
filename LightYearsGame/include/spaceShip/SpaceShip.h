#pragma once
#include <framework/Actor.h>
#include <framework/Core.h>
#include <framework/Delegate.h>	
#include "gameplay/HealthComponent.h"

namespace ly
{
	class World;
	class SpaceShip : public Actor
	{
	public:
		virtual void BeginPlay() override;

		SpaceShip(World* owningWorld, std::string texturePath = "");
		virtual void Tick(float deltaTime) override;
		sf::Vector2f GetVelocity() const { return mVelocity; }
		void SetVelocity(const sf::Vector2f& velocity);
		sf::Vector2f GetVelocity() { return mVelocity; }
		virtual void Shoot();

		void OnHealthChanged(float amt,float health,float maxHealth);

	private:
		sf::Vector2f mVelocity;

		HealthComponent mHealthComponent;
	};
}