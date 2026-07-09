#include "spaceShip/SpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "VFX/Explosion.h" 
#include <algorithm>
#include <cmath>

namespace ly
{
	SpaceShip::SpaceShip(World* owningWorld, const ShipDefinition& shipDef):
		Actor(owningWorld, shipDef.texturePath),
		mHealthComponent{shipDef.health, shipDef.health},
		mBlinkColor{255, 0, 0, 255},
		mBlinkTime{0.f},
		mBlinkDuration{.25f},
		mInvulnerability{ false },
		mExplosionType{ (ExplosionType)shipDef.explosionType },
		mMovementMode{ ShipMovementMode::LegacyVelocity },
		mMovementAttributes{ shipDef.movementAttributes },
		mAngularVelocity{ 0.f }
	{
		SetCollisionLayer(CollisionLayer::None);
	}

	void SpaceShip::BeginPlay()
	{
		Actor::BeginPlay();        
		SetEnablePhysics(true);    
		
		SetupCollisionLayers();
		
		mHealthComponent.onHealthChanged.BindAction(GetWeakPtr(), &SpaceShip::OnHealthChanged);
		mHealthComponent.onTakenDamage.BindAction(GetWeakPtr(), &SpaceShip::OnTakenDamage);
		mHealthComponent.onHealthEmpty.BindAction(GetWeakPtr(), &SpaceShip::Blow);
	}

	void SpaceShip::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		if (mMovementMode == ShipMovementMode::ThrustDrift)
		{
			ApplyThrustDriftDamping(deltaTime);
			ClampThrustDriftVelocity();
		}
		AddActorLocationOffset(mVelocity * deltaTime);    
		UpdateBlink(deltaTime);      
	}

	void SpaceShip::AddShipRelativeThrust(const sf::Vector2f& localThrustInput, float deltaTime)
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

		const sf::Vector2f worldAcceleration =
			GetActorRightDirection() * weightedLocalAcceleration.x +
			GetActorForwardDirection() * weightedLocalAcceleration.y;

		mVelocity += worldAcceleration * deltaTime;
	}

	void SpaceShip::RotateTowardWorldLocation(const sf::Vector2f& worldLocation, float deltaTime)
	{
		if (mMovementMode != ShipMovementMode::ThrustDrift)
		{
			return;
		}

		sf::Vector2f direction = worldLocation - GetActorLocation();
		const float aimDistance = GetVectorLength(direction);
		const float mouseAimDeadZone = std::max(0.f, mMovementAttributes.mouseAimDeadZone.currentValue);

		if (aimDistance <= mouseAimDeadZone)
		{
			const float angularSettleAlpha = 1.f - std::exp(-12.f * deltaTime);
			mAngularVelocity = Lerp(mAngularVelocity, 0.f, angularSettleAlpha);
			return;
		}

		const float targetAngle = RadiansToDegrees(std::atan2(direction.y, direction.x)) + 90.f;
		const float currentAngle = GetActorRotation();
		const float angleDelta = GetShortestAngleDelta(targetAngle, currentAngle);
		const float maxTurnSpeed = std::max(0.f, mMovementAttributes.angularTurnSpeed.currentValue);
		const float responsiveness = std::max(0.f, mMovementAttributes.angularTurnResponsiveness.currentValue);

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

		SetActorRotation(currentAngle + turnAmount);
	}

	void SpaceShip::ApplyThrustDriftDamping(float deltaTime)
	{
		float dampingCoefficient = std::clamp(mMovementAttributes.linearDamping.currentValue, 0.f, 1.f);
		const float dampingScale = std::pow(dampingCoefficient, deltaTime);
		mVelocity *= dampingScale;
	}

	void SpaceShip::ClampThrustDriftVelocity()
	{
		const float maxSpeed = std::max(0.f, mMovementAttributes.maxSpeed.currentValue);
		const float currentSpeed = GetVectorLength(mVelocity);

		if (maxSpeed <= 0.f || currentSpeed <= maxSpeed)
		{
			return;
		}

		mVelocity *= maxSpeed / currentSpeed;

		// TODO(GAS-Lite): If future ship designs need separate forward/reverse/strafe terminal speeds,
		// split this single maxSpeed attribute into direction-aware max speed attributes.
	}

	float SpaceShip::GetShortestAngleDelta(float targetAngle, float currentAngle) const
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



	void SpaceShip::Shoot()
	{
	}

	void SpaceShip::SetupCollisionLayers()
	{
	}

	void SpaceShip::Blink()
	{
		mBlinkTime = mBlinkDuration; 
	}

	void SpaceShip::UpdateBlink(float deltaTime)
	{
		if (mBlinkTime > 0) 
		{
			mBlinkTime -= deltaTime;

			mBlinkTime = mBlinkTime > 0 ? mBlinkTime : 0.f;
			
			GetSprite().value().setColor(LerpColor(sf::Color::White, mBlinkColor, mBlinkTime));
		}
	}

	void SpaceShip::OnHealthChanged(float amt, float health, float maxHealth)
	{
	}
	
	void SpaceShip::OnTakenDamage(float amt, float health, float maxHealth)
	{
		Blink();
	}
	
	void SpaceShip::Blow()
	{
		Explosion::SpawnExplosion(GetWorld(), GetActorLocation(), Explosion::GetPreset(GetExplosionType()));
		Blew();
		Destroy();
	}

	void SpaceShip::Blew()
	{
	}
	
	void SpaceShip::ApplyDamage(float amt)
	{                        
		mHealthComponent.ChangeHealth(-amt);     
	}
}
