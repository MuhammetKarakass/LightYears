#pragma once

#include <framework/ActorComponent.h>
#include <framework/Core.h>
#include "gameConfigs/ship/ShipStructs.h"
#include "gameplay/movement/MovementInfluenceController.h"
#include "gameplay/movement/MovementBurstTypes.h"
#include "gameplay/movement/MovementPolicyController.h"

#include <string>
#include <cstdint>
#include <unordered_set>

namespace vega
{
	class SpaceShip;
		namespace movement
		{
			class MovementInfluenceService;
			class MovementPolicyService;
		}

	enum class ShipMovementMode
	{
		LegacyVelocity,
		ThrustDrift
	};

	// Owns the reusable movement state and integration rules for every ship.
	// Not ticked automatically (mCanEverTick stays false): Tick takes the ship's
	// resolved speed-cap multiplier and must run after Actor::Tick and before
	// UpdateBlink/CombatRuntime, so SpaceShip::Tick drives it explicitly (UE
	// tick-prerequisite equivalent). The owner must be a SpaceShip.
	class MovementComponent : public ActorComponent
	{
		VEGA_CLASS(MovementComponent)
	public:
		MovementComponent(Actor& owner, const ShipDefinition& shipDef);

		void Tick(float deltaTime, float speedCapMultiplier = 1.f);
		void RefreshAttributes();

		void SetMovementMode(ShipMovementMode movementMode) { mMovementMode = movementMode; }
		ShipMovementMode GetMovementMode() const { return mMovementMode; }

		const ShipMovementAttributes& GetAttributes() const { return mMovementAttributes; }
		ShipMovementAttributes& GetAttributes() { return mMovementAttributes; }

		// Multiple independent sources may request this movement rule. The input
		// resolver treats it as active until the final source releases it.
		void SetDirectionalThrustSync(const std::string& sourceId, bool enabled);
		bool HasDirectionalThrustSync() const { return !mDirectionalThrustSyncSources.empty(); }
		// Module-owned thrust shaping, kept apart from the ability sync sources above. thrustBonus adds to the
		// ship thrust multiplier; equalizeDirections makes reverse and strafe follow forward thrust.
		void SetModuleMovementRule(float thrustBonus, bool equalizeDirections);
		bool HasModuleDirectionalThrustSync() const { return mModuleDirectionalThrustSync; }
		float GetModuleThrustBonus() const { return mModuleThrustBonus; }
		float ResolveForwardThrust() const;
		float ResolveReverseThrust() const;
		float ResolveStrafeThrust() const;
		// Returns the current policy-resolved speed cap so movement abilities can
		// enter a constant-speed state without duplicating policy arithmetic.
		float ResolveCurrentSpeedCap(float speedCapMultiplier = 1.f) const;

		float GetCurrentMovementSpeed() const { return mStepSeconds > 0.f ? mStepDistance / mStepSeconds : 0.f; }
		float GetCurrentMovementDistance() const { return mStepDistance; }
		// Discontinuous relocation never enters the integrated movement sample.
		void TeleportTo(const sf::Vector2f& destination);
		const sf::Vector2f& GetTotalTeleportOffset() const { return mTotalTeleportOffset; }
		std::uint64_t GetTeleportRevision() const { return mTeleportRevision; }
		bool WasMovementInterrupted() const { return mMovementInterrupted; }
		const sf::Vector2f& GetCurrentMovementStartLocation() const { return mStepStartLocation; }
		void SetLegacySpeed(const sf::Vector2f& speed) { mBaseLegacySpeed = speed; }
		const sf::Vector2f& GetLegacySpeed() const { return mBaseLegacySpeed; }
		sf::Vector2f ResolveLegacySpeed() const { return ResolveLegacySpeed(mBaseLegacySpeed); }
		sf::Vector2f ResolveLegacySpeed(const sf::Vector2f& baseSpeed) const;

		void AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime);
		void AddWorldAcceleration(const sf::Vector2f& worldAcceleration, float deltaTime);
		void SetAbilityWorldMovementInput(const sf::Vector2f& input) { mAbilityWorldMovementInput = input; }
		const sf::Vector2f& GetAbilityWorldMovementInput() const { return mAbilityWorldMovementInput; }
		sf::Vector2f ResolveMovementBurstDirection() const;
		bool StartMovementBurst(const movement::MovementBurstRequest& request);
		void EndMovementBurst();
		bool IsMovementBurstActive() const { return mIsMovementBurstActive; }
		float GetResolvedMovementBurstDistance() const { return mResolvedMovementBurstDistance; }
		void RotateTowardWorldLocation(
			const sf::Vector2f& worldLocation,
			float deltaTime,
			float turnCapabilityMultiplier = 1.f
		);

	private:
		friend class movement::MovementInfluenceService;
		friend class movement::MovementPolicyService;

		// Gameplay code reaches these through MovementInfluenceService. Keeping
		// them private prevents individual abilities from inventing their own
		// movement integration path.
		void ApplyInfluenceImpulse(const movement::ImpulseRequest& request);
		void ApplyInfluenceAcceleration(
			const sf::Vector2f& acceleration,
			float deltaTime
		);
		void SetInfluenceAccelerationSource(
			const movement::AccelerationSourceRequest& request
		);
		void SetInfluenceForcedMovement(
			const movement::ForcedMovementRequest& request
		);
		void RemoveInfluenceSource(movement::MovementInfluenceSourceId sourceId);

		bool ApplyMovementPolicy(const movement::MovementPolicyRequest& request);
		bool ReleaseMovementPolicy(
			const movement::MovementPolicySourceId& sourceId,
			movement::MovementPolicyReleaseMode releaseMode,
			float normalizationDuration
		);
		void ClearMovementPolicies();
		bool SupportsMovementPolicies() const;

		void ApplyThrustDriftDamping(float deltaTime);
		void MoveOwner(const sf::Vector2f& requestedOffset);
		void ApplyContinuousInfluences(float deltaTime);
		void TickInfluenceImpulses(float deltaTime);
		void ClampThrustDriftVelocity(float speedCapMultiplier);
		bool TickMovementBurst(float deltaTime);
		float GetShortestAngleDelta(float targetAngle, float currentAngle) const;
		float ResolveThrustMultiplier() const;
		bool IsDirectionalThrustEqualized() const { return HasDirectionalThrustSync() || mModuleDirectionalThrustSync; }

		SpaceShip& GetShip() const;

		ShipMovementMode mMovementMode;
		float mStepSeconds = 0.f;
		sf::Vector2f mTotalTeleportOffset{};
		std::uint64_t mTeleportRevision = 0;
		bool mMovementInterrupted = false;
		bool mExternalMovementPending = false;
		float mStepDistance = 0.f;
		sf::Vector2f mStepStartLocation{};
		sf::Vector2f mBaseLegacySpeed;
		ShipMovementAttributes mBaseMovementAttributes;
		ShipMovementAttributes mMovementAttributes;
		float mAngularVelocity;
		sf::Vector2f mAbilityWorldMovementInput{ 0.f, 0.f };
		movement::MovementInfluenceController mMovementInfluences;
		movement::MovementPolicyController mMovementPolicies;
		sf::Vector2f mMovementBurstVelocity{ 0.f, 0.f };
		sf::Vector2f mPreservedMovementBurstVelocity{ 0.f, 0.f };
		float mMovementBurstTimeRemaining = 0.f;
		float mResolvedMovementBurstDistance = 0.f;
		bool mIsMovementBurstActive = false;
		std::unordered_set<std::string> mDirectionalThrustSyncSources;
		float mModuleThrustBonus = 0.f;
		bool mModuleDirectionalThrustSync = false;
	};
}
