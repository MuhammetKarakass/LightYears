#include "player/PlayerMovementComponent.h"

#include "player/PlayerSpaceShip.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include <framework/MathUtility.h>
#include <framework/World.h>

#include <algorithm>
#include <cmath>

namespace ly
{
	PlayerMovementComponent::PlayerMovementComponent(PlayerSpaceShip& owner)
		: mOwner(owner)
	{
	}

	void PlayerMovementComponent::Tick(float deltaTime)
	{
		SetInput();
		ConsumeInput(deltaTime);
	}

	void PlayerMovementComponent::SetSpeed(float speed)
	{
		mOwner.GetMovementComponent().SetLegacySpeed({ speed, speed });
	}

	float PlayerMovementComponent::GetSpeed() const
	{
		return mOwner.GetMovementComponent().GetLegacySpeed().x;
	}

	void PlayerMovementComponent::SetInput()
	{
		mMoveInput = { 0.f, 0.f };
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		{
			mMoveInput.y = 1.f;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			mMoveInput.x = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			mMoveInput.x = 1.f;
		}

		if (mUseScreenClamp)
		{
			ClampInputOnEdge();
		}

		mAfterburnerRequested =
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

		mOwner.GetMovementComponent().SetAbilityWorldMovementInput(
			ResolveAbilityWorldMovementInput()
		);

		CombatRuntime& combatRuntime = mOwner.GetCombatRuntime();
		combatRuntime.GetAbilities().SetSlotInput(
			AbilitySlot::PrimaryFire,
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)
		);
		combatRuntime.GetAbilities().SetSlotInput(
			AbilitySlot::Ability1,
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)
		);
		combatRuntime.GetAbilities().SetSlotInput(
			AbilitySlot::Ability2,
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)
		);
		combatRuntime.GetAbilities().SetSlotInput(
			AbilitySlot::Ability3,
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)
		);
		combatRuntime.GetAbilities().SetSlotInput(
			AbilitySlot::Ability4,
			sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)
		);

		if (mOwner.GetMovementComponent().GetMovementMode() == ShipMovementMode::LegacyVelocity)
		{
			NormalizeInput();
		}
	}

	void PlayerMovementComponent::ConsumeInput(float deltaTime)
	{
		UpdateAfterburnerState(deltaTime);
		MovementComponent& movement = mOwner.GetMovementComponent();
		if (movement.IsDashing())
		{
			mSmoothedMoveInput = { 0.f, 0.f };
			return;
		}
		if (movement.GetMovementMode() == ShipMovementMode::ThrustDrift)
		{
			const ShipMovementAttributes& movementAttributes = movement.GetAttributes();
			const float inputResponsiveness = std::max(
				0.f,
				movementAttributes.inputResponsiveness.currentValue * GetMovementTurnCapabilityMultiplier()
			);
			const float inputResponseAlpha = inputResponsiveness > 0.f
				? 1.f - std::exp(-inputResponsiveness * deltaTime)
				: 1.f;
			mSmoothedMoveInput = LerpVector(mSmoothedMoveInput, mMoveInput, inputResponseAlpha);

			float forwardInput = std::clamp(-mSmoothedMoveInput.y, -1.f, 1.f);
			const float strafeInput = std::clamp(mSmoothedMoveInput.x, -1.f, 1.f);
			const bool hasMovementInput = std::max(std::abs(mMoveInput.x), std::abs(mMoveInput.y)) > 0.001f;
			if (mAfterburnerIsDraining && !hasMovementInput)
			{
				forwardInput = 1.f;
			}

			const float forwardThrust = forwardInput >= 0.f
				? movementAttributes.forwardThrust.currentValue
				: movementAttributes.reverseThrust.currentValue;
			const float strafeThrust = movementAttributes.strafeThrust.currentValue;
			sf::Vector2f worldAcceleration =
				mOwner.GetActorForwardDirection() * forwardInput * forwardThrust +
				GetAdaptiveScreenStrafeDirection() * strafeInput * strafeThrust;

			const float dominantAxisMagnitude = std::max(
				std::abs(forwardInput) * forwardThrust,
				std::abs(strafeInput) * strafeThrust
			);
			const float accelerationLength = GetVectorLength(worldAcceleration);
			if (accelerationLength > dominantAxisMagnitude && dominantAxisMagnitude > 0.f)
			{
				worldAcceleration *= dominantAxisMagnitude / accelerationLength;
			}

			const float afterburnerAccelerationMultiplier = std::max(
				1.f,
				mOwner.GetShipRuntime().GetAfterburnerAccelerationMultiplier()
			);
			movement.AddWorldAcceleration(
				worldAcceleration * Lerp(1.f, afterburnerAccelerationMultiplier, mAfterburnerIntensity),
				deltaTime
			);
			RotateTowardMouseCursor(deltaTime);
		}
		else
		{
			mSmoothedMoveInput = { 0.f, 0.f };
			sf::Vector2f movementDirection = mMoveInput;
			if (GetVectorLength(movementDirection) <= 0.001f && IsAfterburning())
			{
				movementDirection = mOwner.GetActorForwardDirection();
			}

			const float afterburnerSpeedMultiplier = std::max(
				1.f,
				mOwner.GetShipRuntime().GetAfterburnerSpeedMultiplier()
			);
			const sf::Vector2f normalSpeed = movement.ResolveLegacySpeed();
			const float speedMultiplier = Lerp(1.f, afterburnerSpeedMultiplier, mAfterburnerIntensity);
			const sf::Vector2f targetVelocity{
				movementDirection.x * normalSpeed.x * speedMultiplier,
				movementDirection.y * normalSpeed.y * speedMultiplier
			};

			if (IsAfterburning())
			{
				const float accelerationMultiplier = std::max(
					1.f,
					mOwner.GetShipRuntime().GetAfterburnerAccelerationMultiplier()
				);
				const float responseRate = std::max(
					0.f,
					movement.GetAttributes().inputResponsiveness.currentValue * accelerationMultiplier
				);
				const float responseAlpha = responseRate > 0.f
					? 1.f - std::exp(-responseRate * deltaTime)
					: 1.f;
				mOwner.SetVelocity(LerpVector(mOwner.GetVelocity(), targetVelocity, responseAlpha));
			}
			else
			{
				mOwner.SetVelocity(targetVelocity);
			}
		}

		mMoveInput = { 0.f, 0.f };
	}

	void PlayerMovementComponent::UpdateAfterburnerState(float deltaTime)
	{
		mAfterburnerIsDraining = false;
		if (mAfterburnerRequested && deltaTime > 0.f)
		{
			const float drainPerSecond = std::max(
				0.f,
				mOwner.GetShipRuntime().GetAfterburnerEnergyDrainPerSecond()
			);
			const float requestedEnergy = drainPerSecond * deltaTime;
			if (requestedEnergy > 0.f)
			{
				mAfterburnerIsDraining = mOwner.GetEnergyComponent().Consume(requestedEnergy) > 0.f;
			}
		}

		const float targetIntensity = mAfterburnerIsDraining ? 1.f : 0.f;
		const float transitionDuration = targetIntensity > mAfterburnerIntensity
			? std::max(0.f, mOwner.GetShipRuntime().GetAfterburnerRampUpDuration())
			: std::max(0.f, mOwner.GetShipRuntime().GetAfterburnerRampDownDuration());
		const float intensityStep = transitionDuration > 0.f
			? std::clamp(deltaTime / transitionDuration, 0.f, 1.f)
			: 1.f;
		mAfterburnerIntensity = targetIntensity > mAfterburnerIntensity
			? std::min(targetIntensity, mAfterburnerIntensity + intensityStep)
			: std::max(targetIntensity, mAfterburnerIntensity - intensityStep);
	}

	void PlayerMovementComponent::NormalizeInput()
	{
		NormalizeVector(mMoveInput);
	}

	void PlayerMovementComponent::ClampInputOnEdge()
	{
		if (mOwner.GetActorLocation().x - 40.f <= 0.f && mMoveInput.x == -1.f)
		{
			mMoveInput.x = 0.f;
		}
		else if (mOwner.GetActorLocation().x + 40.f >= mOwner.GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;
		}

		if (mOwner.GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;
		}
		else if (mOwner.GetActorLocation().y + 40.f >= mOwner.GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f;
		}
	}

	sf::Vector2f PlayerMovementComponent::GetAdaptiveScreenStrafeDirection() const
	{
		const sf::Vector2f screenRight{ 1.f, 0.f };
		const sf::Vector2f horizontalFacingStrafe{ 0.f, -1.f };
		const sf::Vector2f shipForward = mOwner.GetActorForwardDirection();
		const float horizontalFacingAmount = std::abs(shipForward.x);
		const float blendStart = 0.25f;
		const float blendEnd = 0.85f;
		float blendAlpha = std::clamp((horizontalFacingAmount - blendStart) / (blendEnd - blendStart), 0.f, 1.f);
		blendAlpha = blendAlpha * blendAlpha * (3.f - 2.f * blendAlpha);

		sf::Vector2f strafeDirection = screenRight * (1.f - blendAlpha) + horizontalFacingStrafe * blendAlpha;
		NormalizeVector(strafeDirection);
		return strafeDirection;
	}

	sf::Vector2f PlayerMovementComponent::ResolveAbilityWorldMovementInput() const
	{
		if (mOwner.GetMovementComponent().GetMovementMode() != ShipMovementMode::ThrustDrift)
		{
			return mMoveInput;
		}

		const float forwardInput = std::clamp(-mMoveInput.y, -1.f, 1.f);
		const float strafeInput = std::clamp(mMoveInput.x, -1.f, 1.f);
		return
			mOwner.GetActorForwardDirection() * forwardInput +
			GetAdaptiveScreenStrafeDirection() * strafeInput;
	}

	void PlayerMovementComponent::RotateTowardMouseCursor(float deltaTime)
	{
		if (World* world = mOwner.GetWorld())
		{
			mOwner.GetMovementComponent().RotateTowardWorldLocation(
				world->GetMouseWorldPosition(),
				deltaTime,
				GetMovementTurnCapabilityMultiplier()
			);
		}
	}

	float PlayerMovementComponent::GetMovementSpeedCapMultiplier() const
	{
		const float afterburnerSpeedMultiplier = std::max(
			1.f,
			mOwner.GetShipRuntime().GetAfterburnerSpeedMultiplier()
		);
		return Lerp(1.f, afterburnerSpeedMultiplier, mAfterburnerIntensity);
	}

	float PlayerMovementComponent::GetMovementTurnCapabilityMultiplier() const
	{
		const float maneuverabilityMultiplier = std::clamp(
			mOwner.GetShipRuntime().GetAfterburnerManeuverabilityMultiplier(),
			0.f,
			1.f
		);
		return Lerp(1.f, maneuverabilityMultiplier, mAfterburnerIntensity);
	}

	bool PlayerMovementComponent::IsAfterburnerRechargeBlocked() const
	{
		return IsAfterburning();
	}
}
