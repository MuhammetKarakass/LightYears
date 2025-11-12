#pragma once
#include <framework/Actor.h>
#include <framework/Core.h>
#include <framework/Delegate.h>	
#include "VFX/Explosion.h"
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

		HealthComponent& GetHealthComponent() { return mHealthComponent; }

		virtual void Shoot();
		virtual void ApplyDamage(float amt) override;

		virtual void SetExplosionType(ExplosionType type) { mExplosionType = type; }
		ExplosionType GetExplosionType() const { return mExplosionType; }

		// Derived types override to set faction/layers
		virtual void SetupCollisionLayers();


	private:
		sf::Vector2f mVelocity;
		HealthComponent mHealthComponent;

		sf::Color mBlinkColor;
		float mBlinkTime;
		float mBlinkDuration;

		ExplosionType mExplosionType;

		void Blink();
		void UpdateBlink(float deltaTime);

		virtual void OnHealthChanged(float amt, float health, float maxHealth);
		virtual void OnTakenDamage(float amt, float health, float maxHealth);
		void Blow();
		virtual void Blew();
	};
}