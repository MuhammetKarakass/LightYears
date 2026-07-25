#pragma once

#include <framework/Core.h>
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/ability/dash/DashMovementController.h"

namespace ly
{
	class SpaceShip;

	enum class ShipMovementMode
	{
		LegacyVelocity,
		ThrustDrift
	};

	// Owns the reusable movement state and integration rules for every ship.
	class MovementComponent
	{
	public:
		MovementComponent(SpaceShip& owner, const ShipDefinition& shipDef);

		void Tick(float deltaTime, float speedCapMultiplier = 1.f);
		void RefreshAttributes();

		void SetMovementMode(ShipMovementMode movementMode) { mMovementMode = movementMode; }
		ShipMovementMode GetMovementMode() const { return mMovementMode; }

		const ShipMovementAttributes& GetAttributes() const { return mMovementAttributes; }
		ShipMovementAttributes& GetAttributes() { return mMovementAttributes; }

		void SetLegacySpeed(const sf::Vector2f& speed) { mBaseLegacySpeed = speed; }
		const sf::Vector2f& GetLegacySpeed() const { return mBaseLegacySpeed; }
		sf::Vector2f ResolveLegacySpeed() const { return ResolveLegacySpeed(mBaseLegacySpeed); }
		sf::Vector2f ResolveLegacySpeed(const sf::Vector2f& baseSpeed) const;

		void AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime);
		void AddWorldAcceleration(const sf::Vector2f& worldAcceleration, float deltaTime);
		void SetAbilityWorldMovementInput(const sf::Vector2f& input) { mAbilityWorldMovementInput = input; }
		sf::Vector2f ResolveDashDirection() const;
		bool StartDash(const DashRequest& request);
		void EndDash();
		bool IsDashing() const { return mIsDashing; }
		float GetResolvedDashDistance() const { return mResolvedDashDistance; }
		void RotateTowardWorldLocation(
			const sf::Vector2f& worldLocation,
			float deltaTime,
			float turnCapabilityMultiplier = 1.f
		);

	private:
		void ApplyThrustDriftDamping(float deltaTime);
		void ClampThrustDriftVelocity(float speedCapMultiplier);
		bool TickDash(float deltaTime);
		float GetShortestAngleDelta(float targetAngle, float currentAngle) const;

		SpaceShip& mOwner;
		ShipMovementMode mMovementMode;
		sf::Vector2f mBaseLegacySpeed;
		ShipMovementAttributes mBaseMovementAttributes;
		ShipMovementAttributes mMovementAttributes;
		float mAngularVelocity;
		sf::Vector2f mAbilityWorldMovementInput{ 0.f, 0.f };
		sf::Vector2f mDashVelocity{ 0.f, 0.f };
		sf::Vector2f mPreservedDashVelocity{ 0.f, 0.f };
		float mDashTimeRemaining = 0.f;
		float mResolvedDashDistance = 0.f;
		bool mIsDashing = false;
	};
}
