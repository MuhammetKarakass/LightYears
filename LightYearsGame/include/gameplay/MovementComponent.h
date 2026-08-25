#pragma once

#include <framework/Core.h>
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "gameplay/movement/ForcedMovementRequest.h"

#include <unordered_map>

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
		// External forces (explosions, pulls, future knockbacks) are velocity
		// impulses, not position teleports. They remain active while control
		// effects block player input, then decay naturally.
		inline static constexpr float ExternalImpulseRetentionPerSecond = 0.01f;
		void ApplyExternalImpulse(
			const sf::Vector2f& velocityChange,
			float retentionPerSecond = ExternalImpulseRetentionPerSecond
		);
		// Forced movement is resolved by the movement component so player input,
		// drift and control abilities share one integration path.
		void SetForcedMovementVelocity(
			ForcedMovementSourceId sourceId,
			const sf::Vector2f& velocity
		);
		void RemoveForcedMovementSource(ForcedMovementSourceId sourceId);
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
		void MoveOwner(const sf::Vector2f& requestedOffset);
		void TickExternalImpulse(float deltaTime);
		void ClampThrustDriftVelocity(float speedCapMultiplier);
		bool TickDash(float deltaTime);
		float GetShortestAngleDelta(float targetAngle, float currentAngle) const;

		struct ExternalImpulse
		{
			sf::Vector2f velocity{ 0.f, 0.f };
			float retentionPerSecond = ExternalImpulseRetentionPerSecond;
		};

		SpaceShip& mOwner;
		ShipMovementMode mMovementMode;
		sf::Vector2f mBaseLegacySpeed;
		ShipMovementAttributes mBaseMovementAttributes;
		ShipMovementAttributes mMovementAttributes;
		float mAngularVelocity;
		sf::Vector2f mAbilityWorldMovementInput{ 0.f, 0.f };
		// Each source decays independently. This prevents a soft ability-driven
		// drift from changing the short, sharp knockback of an explosion.
		List<ExternalImpulse> mExternalImpulses;
		std::unordered_map<ForcedMovementSourceId, sf::Vector2f> mForcedMovementVelocities;
		sf::Vector2f mDashVelocity{ 0.f, 0.f };
		sf::Vector2f mPreservedDashVelocity{ 0.f, 0.f };
		float mDashTimeRemaining = 0.f;
		float mResolvedDashDistance = 0.f;
		bool mIsDashing = false;
	};
}
