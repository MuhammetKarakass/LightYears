#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/MovementComponent.h"

#include "gameplay/ability/dash/DashMovementMath.h"
#include "spaceShip/SpaceShip.h"
#include <framework/MathUtility.h>
#include <framework/World.h>

#include <algorithm>
#include <cmath>

namespace ly
{
	MovementComponent::MovementComponent(SpaceShip& owner, const ShipDefinition& shipDef)
		: mOwner(owner)
		, mMovementMode(ShipMovementMode::LegacyVelocity)
		, mBaseLegacySpeed(shipDef.speed)
		, mBaseMovementAttributes(shipDef.movementAttributes)
		, mMovementAttributes(shipDef.movementAttributes)
		, mAngularVelocity(0.f)
	{
	}

	void MovementComponent::Tick(float deltaTime, float speedCapMultiplier)
	{
		if (TickDash(deltaTime))
		{
			return;
		}

		if (mMovementMode == ShipMovementMode::ThrustDrift)
		{
			ApplyThrustDriftDamping(deltaTime);
			ClampThrustDriftVelocity(speedCapMultiplier);
		}

		const float movementMultiplier = mOwner.GetAbilitySystemComponent().GetAttributes()
			.GetSequentialReductionMultiplier(OwnerAttributeIds::MovementSlow, 0.05f);
		mOwner.AddActorLocationOffset(mOwner.GetVelocity() * movementMultiplier * deltaTime);
	}

	void MovementComponent::RefreshAttributes()
	{
		const sas::AttributeSystem& attributes = mOwner.GetAbilitySystemComponent().GetAttributes();
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

	sf::Vector2f MovementComponent::ResolveLegacySpeed(const sf::Vector2f& baseSpeed) const
	{
		const sas::AttributeSystem& attributes = mOwner.GetAbilitySystemComponent().GetAttributes();
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
			? mMovementAttributes.forwardThrust.currentValue
			: mMovementAttributes.reverseThrust.currentValue;
		const float strafeThrust = mMovementAttributes.strafeThrust.currentValue;

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
			mOwner.GetActorRightDirection() * weightedLocalAcceleration.x +
			mOwner.GetActorForwardDirection() * weightedLocalAcceleration.y,
			deltaTime
		);
	}

	void MovementComponent::AddWorldAcceleration(const sf::Vector2f& worldAcceleration, float deltaTime)
	{
		if (mIsDashing)
		{
			return;
		}
		mOwner.SetVelocity(mOwner.GetVelocity() + worldAcceleration * deltaTime);
	}

	sf::Vector2f MovementComponent::ResolveDashDirection() const
	{
		sf::Vector2f direction = mAbilityWorldMovementInput;
		if (GetVectorLength(direction) > 0.001f)
		{
			NormalizeVector(direction);
			return direction;
		}

		if (const World* world = mOwner.GetWorld())
		{
			direction = world->GetMouseWorldPosition() - mOwner.GetActorLocation();
			if (GetVectorLength(direction) > 0.001f)
			{
				NormalizeVector(direction);
				return direction;
			}
		}

		return { 0.f, 0.f };
	}

	bool MovementComponent::StartDash(const DashRequest& request)
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

		const sas::AttributeSystem& attributes = mOwner.GetAbilitySystemComponent().GetAttributes();
		const float horizontalRating = attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal);
		const float verticalRating = attributes.GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical);
		mResolvedDashDistance = DashMovementMath::ResolveDistance(
			request.baseDistance,
			horizontalRating,
			verticalRating
		);
		if (mResolvedDashDistance <= 0.f)
		{
			return false;
		}

		mPreservedDashVelocity = mOwner.GetVelocity();
		mDashVelocity = DashMovementMath::ResolveVelocity(
			direction,
			mResolvedDashDistance,
			request.duration,
			mPreservedDashVelocity
		);
		mDashTimeRemaining = request.duration;
		mIsDashing = true;
		mOwner.SetVelocity(mDashVelocity);
		return true;
	}

	void MovementComponent::EndDash()
	{
		if (mIsDashing)
		{
			mOwner.SetVelocity(mPreservedDashVelocity);
		}
		mIsDashing = false;
		mDashTimeRemaining = 0.f;
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

		sf::Vector2f direction = worldLocation - mOwner.GetActorLocation();
		const float aimDistance = GetVectorLength(direction);
		const float mouseAimDeadZone = std::max(0.f, mMovementAttributes.mouseAimDeadZone.currentValue);
		if (aimDistance <= mouseAimDeadZone)
		{
			const float angularSettleAlpha = 1.f - std::exp(-12.f * deltaTime);
			mAngularVelocity = Lerp(mAngularVelocity, 0.f, angularSettleAlpha);
			return;
		}

		const float targetAngle = RadiansToDegrees(std::atan2(direction.y, direction.x)) + 90.f;
		const float currentAngle = mOwner.GetActorRotation();
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

		mOwner.SetActorRotation(currentAngle + turnAmount);
	}

	void MovementComponent::ApplyThrustDriftDamping(float deltaTime)
	{
		const float dampingCoefficient = std::clamp(mMovementAttributes.linearDamping.currentValue, 0.f, 1.f);
		mOwner.SetVelocity(mOwner.GetVelocity() * std::pow(dampingCoefficient, deltaTime));
	}

	void MovementComponent::ClampThrustDriftVelocity(float speedCapMultiplier)
	{
		const float maxSpeed = std::max(0.f, mMovementAttributes.maxSpeed.currentValue * speedCapMultiplier);
		const float currentSpeed = GetVectorLength(mOwner.GetVelocity());
		if (maxSpeed <= 0.f || currentSpeed <= maxSpeed)
		{
			return;
		}

		mOwner.SetVelocity(mOwner.GetVelocity() * (maxSpeed / currentSpeed));
	}

	bool MovementComponent::TickDash(float deltaTime)
	{
		if (!mIsDashing)
		{
			return false;
		}

		const float stepDuration = std::min(std::max(0.f, deltaTime), mDashTimeRemaining);
		mOwner.SetVelocity(mDashVelocity);
		mOwner.AddActorLocationOffset(mDashVelocity * stepDuration);
		mDashTimeRemaining = std::max(0.f, mDashTimeRemaining - stepDuration);
		if (mDashTimeRemaining <= 0.f)
		{
			EndDash();
		}
		return true;
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
