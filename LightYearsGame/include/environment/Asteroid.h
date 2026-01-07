#pragma once

#include "framework/Actor.h"
#include "player/Reward.h"
#include "VFX/Explosion.h"
#include "gameplay/HealthComponent.h"

namespace ly
{
	class Asteroid : public Actor
	{
	public:
		Asteroid(World* owningWorld,const sf::Vector2f& velocity = sf::Vector2f(0.f, 100.f), float health = 50.f, float damage = 20.f);

		virtual void BeginPlay() override;

		virtual void ApplyDamage(float amt) override;

		void SetExplosionType(ExplosionType type) { mExplosionType = type; }
		ExplosionType GetExplosionType() const { return mExplosionType; }
		void SetupCollisionLayers();
		void SetAsteroidVelocity(const sf::Vector2f& velocity) { mVelocity = velocity; }
		void SetAsteroidScale(float scale);
	private:
		virtual void Tick(float deltaTime) override;
		virtual	void OnActorBeginOverlap(Actor* otherActor) override;
		void Blink();
		void UpdateBlink(float deltaTime);
		void SpawnReward();
		void OnHealthChanged(float amt, float health, float maxHealth);
		void OnTakenDamage(float amt, float health, float maxHealth);
		void Blow();

		sf::Vector2f mVelocity;
		float mDamage;
		ExplosionType mExplosionType;
		List<WeightedReward> mRewards;
		HealthComponent mHealthComponent;
		sf::Color mBlinkColor;
		float mBlinkTime;
		float mBlinkDuration;
		float mRotationSpeed;
		List<std::string> mTexturePaths;
	};
}