#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/tags/GameplayTags.h"
#include <framework/reflection/Reflection.h>

#include "gameplay/movement/MovementBurstMath.h"
#include "spaceShip/SpaceShip.h"
#include <framework/MathUtility.h>
#include <framework/World.h>

#include <algorithm>
#include <cmath>

namespace vega
{
	VEGA_REFLECT(MovementComponent, ActorComponent)
	{
		VEGA_CATEGORY("Movement");
		VEGA_PROPERTY("Movement Mode", GetMovementMode, SetMovementMode).Labels({ { 0, "LegacyVelocity" }, { 1, "ThrustDrift" } });
		VEGA_PROPERTY("Legacy Speed", GetLegacySpeed, SetLegacySpeed);
		VEGA_READONLY("Current Movement Speed", GetCurrentMovementSpeed);
		VEGA_READONLY("Current Movement Distance", GetCurrentMovementDistance);
		VEGA_READONLY("Current Movement Start Location", GetCurrentMovementStartLocation);
		VEGA_READONLY("Directional Thrust Sync", HasDirectionalThrustSync);
		VEGA_READONLY("Module Directional Thrust Sync", HasModuleDirectionalThrustSync);
		VEGA_READONLY("Module Thrust Bonus", GetModuleThrustBonus);
		VEGA_READONLY("Movement Burst Active", IsMovementBurstActive);
		VEGA_READONLY("Resolved Movement Burst Distance", GetResolvedMovementBurstDistance);
	}

	MovementComponent::MovementComponent(Actor& owner, const ShipDefinition& shipDef)
		: ActorComponent(owner)
		, mMovementMode(ShipMovementMode::LegacyVelocity)
		, mBaseLegacySpeed(shipDef.speed)
		, mBaseMovementAttributes(shipDef.movementAttributes)
		, mMovementAttributes(shipDef.movementAttributes)
		, mAngularVelocity(0.f)
	{
	}

	SpaceShip& MovementComponent::GetShip() const
	{
		return static_cast<SpaceShip&>(GetOwner());
	}

	void MovementComponent::Tick(float deltaTime, float speedCapMultiplier)
	{
		mStepSeconds = std::max(0.f, deltaTime);
		mStepDistance = 0.f;
		mStepStartLocation = GetShip().GetActorLocation();
		mMovementPolicies.Tick(deltaTime);
		const auto& ownedTags = GetShip().GetAbilitySystemComponent().GetOwnedTags();
		const auto applyForcedMovement = [&]()
		{
			const sf::Vector2f forcedVelocity =
				mMovementInfluences.ResolveForcedVelocity();
			if (GetVectorLength(forcedVelocity) > 0.001f)
			{
				MoveOwner(forcedVelocity * std::max(0.f, deltaTime));
			}
		};
		if (ownedTags.HasTag(GameplayTags::Status::Damage::Cryo::Frozen) ||
			(ownedTags.HasTag(GameplayTags::State::ActionLock::MovementInput) && ownedTags.HasTag(GameplayTags::State::ActionLock::ExternalMovement)))
		{
			// Frozen status and hard stasis stop existing motion and external influences.
			if (mIsMovementBurstActive)
			{
				EndMovementBurst();
			}
			GetShip().SetVelocity({ 0.f, 0.f });
			mMovementInfluences.ClearImpulses();
			mMovementInfluences.ClearAccelerationSources();
			return;
		}
		if (ownedTags.HasTag(GameplayTags::State::ActionLock::MovementInput))
		{

			if (mMovementInfluences.HasForcedMovement())
			{
				// A movement ability such as Blastback recoil owns translation for
				// its authored window, even though it also blocks new player input.
				if (mIsMovementBurstActive)
				{
					EndMovementBurst();
				}
				GetShip().SetVelocity({ 0.f, 0.f });
				applyForcedMovement();
				TickInfluenceImpulses(deltaTime);
				return;
			}

			// MovementInput means “do not accept a new command”, not “freeze the
			// ship”. Preserve prior drift, natural damping and external influences
			// throughout focus so a concentrating ship can still be pushed or pulled.
			if (TickMovementBurst(deltaTime))
			{
				TickInfluenceImpulses(deltaTime);
				return;
			}
			ApplyContinuousInfluences(deltaTime);
			if (mMovementMode == ShipMovementMode::ThrustDrift)
			{
				ApplyThrustDriftDamping(deltaTime);
				ClampThrustDriftVelocity(speedCapMultiplier);
			}
			const float movementMultiplier = GetShip().GetAbilitySystemComponent().GetAttributes()
				.GetSequentialReductionMultiplier(OwnerAttributeIds::MovementSlow, 0.05f);
			MoveOwner(GetShip().GetVelocity() * movementMultiplier * std::max(0.f, deltaTime));
			TickInfluenceImpulses(deltaTime);
			return;
		}

		if (mMovementInfluences.HasForcedMovement())
		{
			// A forced source owns translational movement but not aim/rotation or
			// combat. Movement abilities are blocked by the shared input lock while
			// this branch keeps the ship moving through the normal component.
			if (mIsMovementBurstActive)
			{
				EndMovementBurst();
			}
			GetShip().SetVelocity({ 0.f, 0.f });
			applyForcedMovement();
			TickInfluenceImpulses(deltaTime);
			return;
		}

		if (ownedTags.HasTag(GameplayTags::State::Effect::Control::Stunned) ||
			ownedTags.HasTag(GameplayTags::State::Effect::Control::Staggered))
		{
			if (mIsMovementBurstActive)
			{
				EndMovementBurst();
			}
			GetShip().SetVelocity({ 0.f, 0.f });
			TickInfluenceImpulses(deltaTime);
			return;
		}

		if (TickMovementBurst(deltaTime))
		{
			return;
		}

		ApplyContinuousInfluences(deltaTime);

		if (mMovementMode == ShipMovementMode::ThrustDrift)
		{
			ApplyThrustDriftDamping(deltaTime);
			ClampThrustDriftVelocity(speedCapMultiplier);
		}

		const float movementMultiplier = GetShip().GetAbilitySystemComponent().GetAttributes()
			.GetSequentialReductionMultiplier(OwnerAttributeIds::MovementSlow, 0.05f);
		MoveOwner(GetShip().GetVelocity() * movementMultiplier * deltaTime);
		TickInfluenceImpulses(deltaTime);
	}

	void MovementComponent::ApplyInfluenceImpulse(
		const movement::ImpulseRequest& request
	)
	{
		mMovementInfluences.ApplyImpulse(request);
	}

	void MovementComponent::ApplyInfluenceAcceleration(
		const sf::Vector2f& acceleration,
		float deltaTime
	)
	{
		GetShip().SetVelocity(GetShip().GetVelocity() + acceleration * std::max(0.f, deltaTime));
	}

	void MovementComponent::SetInfluenceAccelerationSource(
		const movement::AccelerationSourceRequest& request
	)
	{
		mMovementInfluences.SetAccelerationSource(request);
	}

	void MovementComponent::SetInfluenceForcedMovement(
		const movement::ForcedMovementRequest& request
	)
	{
		mMovementInfluences.SetForcedMovement(request);
	}

	void MovementComponent::RemoveInfluenceSource(
		movement::MovementInfluenceSourceId sourceId
	)
	{
		mMovementInfluences.RemoveSource(sourceId);
	}

	bool MovementComponent::ApplyMovementPolicy(
		const movement::MovementPolicyRequest& request
	)
	{
		return mMovementPolicies.SetPolicy(request);
	}

	bool MovementComponent::ReleaseMovementPolicy(
		const movement::MovementPolicySourceId& sourceId,
		movement::MovementPolicyReleaseMode releaseMode,
		float normalizationDuration
	)
	{
		return mMovementPolicies.RemovePolicy(
			sourceId,
			releaseMode,
			GetVectorLength(GetShip().GetVelocity()),
			normalizationDuration
		);
	}

	void MovementComponent::ClearMovementPolicies()
	{
		mMovementPolicies.ClearPolicies();
	}

	bool MovementComponent::SupportsMovementPolicies() const
	{
		// LegacyVelocity replaces velocity from input before this component ticks;
		// it cannot preserve momentum-based policies reliably.
		return mMovementMode == ShipMovementMode::ThrustDrift;
	}

	void MovementComponent::ApplyContinuousInfluences(float deltaTime)
	{
		const sf::Vector2f acceleration = mMovementInfluences.ResolveAcceleration();
		if (GetVectorLength(acceleration) > 0.001f)
		{
			ApplyInfluenceAcceleration(acceleration, deltaTime);
		}
	}

	void MovementComponent::TickInfluenceImpulses(float deltaTime)
	{
		for (const sf::Vector2f& offset : mMovementInfluences.AdvanceImpulses(deltaTime))
		{
			MoveOwner(offset);
		}
	}

	void MovementComponent::RefreshAttributes()
	{
		const sas::AttributeSystem& attributes = GetShip().GetAbilitySystemComponent().GetAttributes();
		mMovementAttributes = mBaseMovementAttributes;

		const float horizontalRating = attributes.HasAttribute(OwnerAttributeIds::MoveSpeedHorizontal)
			? std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal))
			: 0.f;
		const float verticalRating = attributes.HasAttribute(OwnerAttributeIds::MoveSpeedVertical)
			? std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical))
			: 0.f;

		mMovementAttributes.strafeThrust.currentValue =
			mBaseMovementAttributes.strafeThrust.currentValue +
			mBaseMovementAttributes.GetDiminishingRatingContribution(
				horizontalRating,
				mBaseMovementAttributes.horizontalRatingToThrust
			);
		mMovementAttributes.forwardThrust.currentValue =
			mBaseMovementAttributes.forwardThrust.currentValue +
			mBaseMovementAttributes.GetDiminishingRatingContribution(
				verticalRating,
				mBaseMovementAttributes.verticalRatingToThrust
			);
		mMovementAttributes.reverseThrust.currentValue =
			mBaseMovementAttributes.reverseThrust.currentValue +
			mBaseMovementAttributes.GetDiminishingRatingContribution(
				verticalRating,
				mBaseMovementAttributes.verticalRatingToThrust
			);
		mMovementAttributes.maxSpeed.currentValue =
			mBaseMovementAttributes.maxSpeed.currentValue +
			mBaseMovementAttributes.GetDiminishingRatingContribution(
				std::max(horizontalRating, verticalRating),
				mBaseMovementAttributes.ratingToMaxSpeed
			);
	}

	void MovementComponent::SetDirectionalThrustSync(
		const std::string& sourceId,
		bool enabled
	)
	{
		if (sourceId.empty())
		{
			return;
		}
		if (enabled)
		{
			mDirectionalThrustSyncSources.insert(sourceId);
		}
		else
		{
			mDirectionalThrustSyncSources.erase(sourceId);
		}
	}

	void MovementComponent::SetModuleMovementRule(float thrustBonus, bool equalizeDirections)
	{
		mModuleThrustBonus = thrustBonus;
		mModuleDirectionalThrustSync = equalizeDirections;
	}

	float MovementComponent::ResolveThrustMultiplier() const
	{
		// Same clamp as SpaceShip::GetThrustMultiplier, with the module bonus added before the clamp.
		return std::max(0.f, 1.f + GetShip().GetRuntimeModifiers().GetThrustBonus() + mModuleThrustBonus);
	}

	float MovementComponent::ResolveForwardThrust() const
	{
		return mMovementAttributes.forwardThrust.currentValue * ResolveThrustMultiplier();
	}

	float MovementComponent::ResolveReverseThrust() const
	{
		return IsDirectionalThrustEqualized()
			? ResolveForwardThrust()
			: mMovementAttributes.reverseThrust.currentValue * ResolveThrustMultiplier();
	}

	float MovementComponent::ResolveStrafeThrust() const
	{
		return IsDirectionalThrustEqualized()
			? ResolveForwardThrust()
			: mMovementAttributes.strafeThrust.currentValue * ResolveThrustMultiplier();
	}

	float MovementComponent::ResolveCurrentSpeedCap(float speedCapMultiplier) const
	{
		return mMovementPolicies.ResolveSpeedCap(
			mMovementAttributes.maxSpeed.currentValue * std::max(0.f, 1.f + mModuleThrustBonus),
			speedCapMultiplier
		);
	}

	sf::Vector2f MovementComponent::ResolveLegacySpeed(const sf::Vector2f& baseSpeed) const
	{
		const sas::AttributeSystem& attributes = GetShip().GetAbilitySystemComponent().GetAttributes();
		const float horizontalRating = attributes.HasAttribute(OwnerAttributeIds::MoveSpeedHorizontal)
			? std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal))
			: 0.f;
		const float verticalRating = attributes.HasAttribute(OwnerAttributeIds::MoveSpeedVertical)
			? std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical))
			: 0.f;

		return {
			baseSpeed.x + mBaseMovementAttributes.GetDiminishingRatingContribution(
				horizontalRating,
				mBaseMovementAttributes.ratingToMaxSpeed
			),
			baseSpeed.y + mBaseMovementAttributes.GetDiminishingRatingContribution(
				verticalRating,
				mBaseMovementAttributes.ratingToMaxSpeed
			)
		};
	}

	void MovementComponent::AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime)
	{
		if (mMovementMode != ShipMovementMode::ThrustDrift)
		{
			return;
		}

		const float strafeInput = std::clamp(localThrustInput.x, -1.f, 1.f);
		const float forwardInput = std::clamp(localThrustInput.y, -1.f, 1.f);
		const float forwardThrust = forwardInput >= 0.f
			? ResolveForwardThrust()
			: ResolveReverseThrust();
		const float strafeThrust = ResolveStrafeThrust();

		sf::Vector2f weightedLocalAcceleration{
			strafeInput * strafeThrust,
			forwardInput * forwardThrust
		};
		const float dominantAxisMagnitude = std::max(
			std::abs(strafeInput) * strafeThrust,
			std::abs(forwardInput) * forwardThrust
		);
		const float weightedLength = GetVectorLength(weightedLocalAcceleration);
		if (weightedLength > dominantAxisMagnitude && dominantAxisMagnitude > 0.f)
		{
			weightedLocalAcceleration *= dominantAxisMagnitude / weightedLength;
		}

		AddWorldAcceleration(
			GetShip().GetActorRightDirection() * weightedLocalAcceleration.x +
			GetShip().GetActorForwardDirection() * weightedLocalAcceleration.y,
			deltaTime
		);
	}

	void MovementComponent::AddWorldAcceleration(const sf::Vector2f& worldAcceleration, float deltaTime)
	{
		if (mIsMovementBurstActive)
		{
			return;
		}
		GetShip().SetVelocity(GetShip().GetVelocity() + worldAcceleration * deltaTime);
	}

	sf::Vector2f MovementComponent::ResolveMovementBurstDirection() const
	{
		sf::Vector2f direction = mAbilityWorldMovementInput;
		if (GetVectorLength(direction) > 0.001f)
		{
			NormalizeVector(direction);
			return direction;
		}

		if (const World* world = GetShip().GetWorld())
		{
			direction = world->GetMouseWorldPosition() - GetShip().GetActorLocation();
			if (GetVectorLength(direction) > 0.001f)
			{
				NormalizeVector(direction);
				return direction;
			}
		}

		return { 0.f, 0.f };
	}

	bool MovementComponent::StartMovementBurst(
		const movement::MovementBurstRequest& request
	)
	{
		if (request.baseDistance <= 0.f || request.duration <= 0.f)
		{
			return false;
		}

		sf::Vector2f direction = request.direction;
		if (GetVectorLength(direction) <= 0.001f)
		{
			return false;
		}
		NormalizeVector(direction);

		const sas::AttributeSystem& attributes = GetShip().GetAbilitySystemComponent().GetAttributes();
		const float horizontalRating = attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal);
		const float verticalRating = attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical);
		mResolvedMovementBurstDistance = request.useResolvedDistance
			? request.baseDistance
			: movement::MovementBurstMath::ResolveDistance(
				request.baseDistance, horizontalRating, verticalRating);
		if (mResolvedMovementBurstDistance <= 0.f)
		{
			return false;
		}

		mPreservedMovementBurstVelocity = GetShip().GetVelocity();
		mMovementBurstVelocity = movement::MovementBurstMath::ResolveVelocity(
			direction,
			mResolvedMovementBurstDistance,
			request.duration,
			mPreservedMovementBurstVelocity
		);
		mMovementBurstTimeRemaining = request.duration;
		mIsMovementBurstActive = true;
		GetShip().SetVelocity(mMovementBurstVelocity);
		return true;
	}

	void MovementComponent::EndMovementBurst()
	{
		if (mIsMovementBurstActive)
		{
			GetShip().SetVelocity(mPreservedMovementBurstVelocity);
		}
		mIsMovementBurstActive = false;
		mMovementBurstTimeRemaining = 0.f;
	}

	void MovementComponent::RotateTowardWorldLocation(
		const sf::Vector2f& worldLocation,
		float deltaTime,
		float turnCapabilityMultiplier)
	{
		if (mMovementMode != ShipMovementMode::ThrustDrift)
		{
			return;
		}

		sf::Vector2f direction = worldLocation - GetShip().GetActorLocation();
		const float aimDistance = GetVectorLength(direction);
		const float mouseAimDeadZone = std::max(0.f, mMovementAttributes.mouseAimDeadZone.currentValue);
		if (aimDistance <= mouseAimDeadZone)
		{
			const float angularSettleAlpha = 1.f - std::exp(-12.f * deltaTime);
			mAngularVelocity = Lerp(mAngularVelocity, 0.f, angularSettleAlpha);
			return;
		}

		const float targetAngle = RadiansToDegrees(std::atan2(direction.y, direction.x)) + 90.f;
		const float currentAngle = GetShip().GetActorRotation();
		const float angleDelta = GetShortestAngleDelta(targetAngle, currentAngle);
		const float maneuverability = std::max(0.f, turnCapabilityMultiplier);
		const float maxTurnSpeed = std::max(0.f, mMovementAttributes.angularTurnSpeed.currentValue) * maneuverability;
		const float responsiveness = std::max(0.f, mMovementAttributes.angularTurnResponsiveness.currentValue) * maneuverability;
		if (maxTurnSpeed <= 0.f || responsiveness <= 0.f || deltaTime <= 0.f)
		{
			mAngularVelocity = 0.f;
			return;
		}

		if (std::abs(angleDelta) <= 0.15f && std::abs(mAngularVelocity) <= 1.f)
		{
			mAngularVelocity = 0.f;
			return;
		}

		const float desiredAngularVelocity = std::clamp(angleDelta * responsiveness, -maxTurnSpeed, maxTurnSpeed);
		const float responseAlpha = 1.f - std::exp(-responsiveness * deltaTime);
		mAngularVelocity = Lerp(mAngularVelocity, desiredAngularVelocity, responseAlpha);

		float turnAmount = mAngularVelocity * deltaTime;
		if (std::abs(turnAmount) > std::abs(angleDelta))
		{
			turnAmount = angleDelta;
			mAngularVelocity = 0.f;
		}

		GetShip().SetActorRotation(currentAngle + turnAmount);
	}

	void MovementComponent::ApplyThrustDriftDamping(float deltaTime)
	{
		const float dampingCoefficient = mMovementPolicies.ResolveDampingRetention(
			mMovementAttributes.linearDamping.currentValue
		);
		GetShip().SetVelocity(GetShip().GetVelocity() * std::pow(dampingCoefficient, deltaTime));
	}

	void MovementComponent::ClampThrustDriftVelocity(float speedCapMultiplier)
	{
		const float maxSpeed = ResolveCurrentSpeedCap(speedCapMultiplier);
		const float currentSpeed = GetVectorLength(GetShip().GetVelocity());
		if (maxSpeed <= 0.f || currentSpeed <= maxSpeed)
		{
			return;
		}

		GetShip().SetVelocity(GetShip().GetVelocity() * (maxSpeed / currentSpeed));
	}

	bool MovementComponent::TickMovementBurst(float deltaTime)
	{
		if (!mIsMovementBurstActive)
		{
			return false;
		}

		const float stepDuration = std::min(
			std::max(0.f, deltaTime),
			mMovementBurstTimeRemaining
		);
		GetShip().SetVelocity(mMovementBurstVelocity);
		MoveOwner(mMovementBurstVelocity * stepDuration);
		mMovementBurstTimeRemaining = std::max(
			0.f,
			mMovementBurstTimeRemaining - stepDuration
		);
		if (mMovementBurstTimeRemaining <= 0.f)
		{
			EndMovementBurst();
		}
		return true;
	}

	void MovementComponent::MoveOwner(const sf::Vector2f& requestedOffset)
	{
		const sf::Vector2f offset = movement::ConstrainMovementAgainstStaticGeometry(GetShip(), requestedOffset);
		mStepDistance += GetVectorLength(offset);
		GetShip().AddActorLocationOffset(offset);
	}

	float MovementComponent::GetShortestAngleDelta(float targetAngle, float currentAngle) const
	{
		float delta = targetAngle - currentAngle;
		while (delta > 180.f)
		{
			delta -= 360.f;
		}
		while (delta < -180.f)
		{
			delta += 360.f;
		}
		return delta;
	}
}
