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
	enum class ShipMovementMode
	{
		LegacyVelocity,
		ThrustDrift
	};

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

		void SetMovementMode(ShipMovementMode movementMode) { mMovementMode = movementMode; }
		ShipMovementMode GetMovementMode() const { return mMovementMode; }

		const ShipMovementAttributes& GetMovementAttributes() const { return mMovementAttributes; }
		ShipMovementAttributes& GetMovementAttributes() { return mMovementAttributes; }

		void AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime);
		void RotateTowardWorldLocation(const sf::Vector2f& worldLocation, float deltaTime);

		virtual void SetupCollisionLayers();

		List<GameplayTag> mGameplayTags;
	private:
		HealthComponent mHealthComponent;

		sf::Color mBlinkColor;
		float mBlinkTime;
		float mBlinkDuration;

		bool mInvulnerability;

		ExplosionType mExplosionType;
		ShipMovementMode mMovementMode;
		ShipMovementAttributes mMovementAttributes;
		float mAngularVelocity;

		void Blink();
		void UpdateBlink(float deltaTime);
		void ApplyThrustDriftDamping(float deltaTime);
		void ClampThrustDriftVelocity();
		float GetShortestAngleDelta(float targetAngle, float currentAngle) const;


		virtual void OnHealthChanged(float amt, float health, float maxHealth);
		virtual void OnTakenDamage(float amt, float health, float maxHealth);
		void Blow();
		virtual void Blew();
	};
}
