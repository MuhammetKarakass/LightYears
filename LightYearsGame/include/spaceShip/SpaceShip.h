#pragma once
#include <framework/Actor.h>
#include <framework/Core.h>
#include <framework/Delegate.h>	
#include "VFX/Explosion.h"
#include "gameplay/HealthComponent.h"
#include "gameConfigs/GameplayConfig.h"
#include "gameConfigs/GameplayStructs.h"

namespace ly
{
	class World;
	class SpaceShip : public Actor
	{
	public:
		virtual void BeginPlay() override;

		SpaceShip(World* owningWorld, const ShipDefinition& shipDef);

		virtual void Tick(float deltaTime) override;


		HealthComponent& GetHealthComponent() { return mHealthComponent; }

		virtual void Shoot();
		virtual void ApplyDamage(float amt) override;

		virtual void SetExplosionType(ExplosionType type) { mExplosionType = type; }
		ExplosionType GetExplosionType() const { return mExplosionType; }

		bool IsInvulnerable() const { return mInvulnerability; }
		void SetInvulnerability(bool invuln) { mInvulnerability = invuln; }

		virtual void SetupCollisionLayers();

		List<GameplayTag> mGameplayTags;
	private:
		HealthComponent mHealthComponent;

		sf::Color mBlinkColor;
		float mBlinkTime;
		float mBlinkDuration;

		bool mInvulnerability;

		ExplosionType mExplosionType;

		void Blink();
		void UpdateBlink(float deltaTime);


		virtual void OnHealthChanged(float amt, float health, float maxHealth);
		virtual void OnTakenDamage(float amt, float health, float maxHealth);
		void Blow();
		virtual void Blew();
	};
}