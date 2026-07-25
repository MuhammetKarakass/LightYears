#pragma once
#include <framework/Actor.h>
#include <framework/Core.h>
#include <framework/Delegate.h>	
#include "VFX/Explosion.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "gameConfigs/ship/ShipStructs.h"

namespace ly
{
	class World;

	class SpaceShip : public Actor, public Combatant, public DashMovementController
	{
	public:
		virtual void BeginPlay() override;

		SpaceShip(World* owningWorld, const ShipDefinition& shipDef);

		virtual void Tick(float deltaTime) override;


		HealthComponent& GetHealthComponent() { return mHealthComponent; }
		const HealthComponent& GetHealthComponent() const { return mHealthComponent; }
		ShieldComponent& GetShieldComponent() { return mShieldComponent; }
		const ShieldComponent& GetShieldComponent() const { return mShieldComponent; }
		EnergyComponent& GetEnergyComponent() { return mEnergyComponent; }
		const EnergyComponent& GetEnergyComponent() const { return mEnergyComponent; }
		CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		ShipRuntime& GetShipRuntime() { return mShipRuntime; }
		const ShipRuntime& GetShipRuntime() const { return mShipRuntime; }

		virtual void Shoot();
		virtual void ApplyDamage(float amt) override;
		virtual void ReceiveDamage(DamageContext context) override;

		virtual void SetExplosionType(ExplosionType type) { mExplosionType = type; }
		ExplosionType GetExplosionType() const { return mExplosionType; }

		bool IsInvulnerable() const { return mInvulnerability; }
		void SetInvulnerability(bool invuln) { mInvulnerability = invuln; }

		MovementComponent& GetMovementComponent() { return mMovementComponent; }
		const MovementComponent& GetMovementComponent() const { return mMovementComponent; }
		void SetMovementMode(ShipMovementMode movementMode) { mMovementComponent.SetMovementMode(movementMode); }
		ShipMovementMode GetMovementMode() const { return mMovementComponent.GetMovementMode(); }

		const ShipMovementAttributes& GetMovementAttributes() const { return mMovementComponent.GetAttributes(); }
		ShipMovementAttributes& GetMovementAttributes() { return mMovementComponent.GetAttributes(); }

		void AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime);
		void RotateTowardWorldLocation(const sf::Vector2f& worldLocation, float deltaTime);
		void RefreshMovementAttributesFromRuntime();

		sf::Vector2f ResolveDashDirection() const override;
		bool StartDash(const DashRequest& request) override;
		void EndDash() override;

		virtual void SetupCollisionLayers();

		List<GameplayTag> mAttachedLightTags;

	protected:
		virtual float GetMovementSpeedCapMultiplier() const { return 1.f; }
		virtual float GetMovementTurnCapabilityMultiplier() const { return 1.f; }
		virtual bool IsAfterburnerRechargeBlocked() const { return false; }
		sf::Vector2f ResolveLegacyMovementSpeed(const sf::Vector2f& baseSpeed) const;

	private:
		HealthComponent mHealthComponent;
		ShieldComponent mShieldComponent;
		EnergyComponent mEnergyComponent;
		CombatRuntime mCombatRuntime;
		ShipRuntime mShipRuntime;
		MovementComponent mMovementComponent;

		sf::Color mBlinkColor;
		float mBlinkTime;
		float mBlinkDuration;

		bool mInvulnerability;

		ExplosionType mExplosionType;
		float mHealthRegenDelayRemaining = 0.f;

		void Blink();
		void UpdateBlink(float deltaTime);
		void UpdateRegeneration(float deltaTime);
		void OnRuntimeAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue);
		void OnShipAttributeChanged(GameplayTag attributeId, float previousValue, float currentValue);


		virtual void OnHealthChanged(float amt, float health, float maxHealth);
		virtual void OnTakenDamage(float amt, float health, float maxHealth);
		void Blow();
		virtual void Blew();
	};
}


