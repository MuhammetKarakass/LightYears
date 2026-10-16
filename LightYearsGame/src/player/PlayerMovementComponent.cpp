#include "player/PlayerMovementComponent.h"
#include <framework/reflection/Reflection.h>

#include "player/PlayerSpaceShip.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/input/AbilityInputSchema.h"
#include <framework/MathUtility.h>
#include <framework/World.h>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace ly
{
	VEGA_REFLECT(PlayerMovementComponent, ActorComponent)
	{
		VEGA_CATEGORY("Player Movement");
		VEGA_PROPERTY("Speed", GetSpeed, SetSpeed).Range(0.f, 100000.f);
		VEGA_PROPERTY("Use Screen Clamp", GetUseScreenClamp, SetUseScreenClamp);
		VEGA_READONLY("Afterburning", IsAfterburning);
		VEGA_READONLY("Afterburner Camera Zoom Out", GetAfterburnerCameraZoomOut);
		VEGA_READONLY("Speed Cap Multiplier", GetMovementSpeedCapMultiplier);
		VEGA_READONLY("Turn Capability Multiplier", GetMovementTurnCapabilityMultiplier);
		VEGA_READONLY("Afterburner Recharge Blocked", IsAfterburnerRechargeBlocked);
	}

	PlayerMovementComponent::PlayerMovementComponent(Actor& owner)
		: ActorComponent(owner)
	{
	}

	PlayerSpaceShip& PlayerMovementComponent::GetPlayer() const
	{
		return static_cast<PlayerSpaceShip&>(GetOwner());
	}

	void PlayerMovementComponent::Tick(float deltaTime)
	{
		if (GetPlayer().IsInPortalTransit())
		{
			mMoveInput = { 0.f, 0.f };
			mSmoothedMoveInput = { 0.f, 0.f };
			mAfterburnerRequested = false;
			mAfterburnerIsActive = false;
			mAfterburnerIntensity = 0.f;
			return;
		}

		SetInput();
		if (GetPlayer().GetAbilitySystemComponent().HasOwnedTag(
			GameplayTags::State::Effect::Control::Stunned
		))
		{
			mMoveInput = { 0.f, 0.f };
			mSmoothedMoveInput = { 0.f, 0.f };
			mAfterburnerRequested = false;
			mAfterburnerIsActive = false;
			mAfterburnerIntensity = 0.f;
			GetPlayer().SetVelocity({ 0.f, 0.f });
			return;
		}
		ConsumeInput(deltaTime);
	}

	void PlayerMovementComponent::SetSpeed(float speed)
	{
		GetPlayer().GetMovementComponent().SetLegacySpeed({ speed, speed });
	}

	float PlayerMovementComponent::GetSpeed() const
	{
		return GetPlayer().GetMovementComponent().GetLegacySpeed().x;
	}

	namespace
	{
		bool gDevAutoPlay = false;
	}

	void PlayerMovementComponent::SetDevAutoPlay(bool enabled)
	{
		gDevAutoPlay = enabled;
	}

	void PlayerMovementComponent::SetInput()
	{
		if (gDevAutoPlay)
		{
			static const auto start = std::chrono::steady_clock::now();
			const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
			mMoveInput = { std::cos(seconds * 0.8f), std::sin(seconds * 0.8f) };
			mAfterburnerRequested = std::fmod(seconds, 6.f) < 1.f;
			GetPlayer().GetMovementComponent().SetAbilityWorldMovementInput(ResolveAbilityWorldMovementInput());
			CombatRuntime& combatRuntime = GetPlayer().GetCombatRuntime();
			for (const AbilityInputBinding& binding : AbilityInputSchema::GetBindings())
			{
				// Primary fire is held; ability keys are tapped so every press is a new activation attempt.
				const bool pressed = binding.slot == sas::AbilitySlot::PrimaryFire || std::fmod(seconds, 0.4f) < 0.2f;
				combatRuntime.GetAbilitySystemComponent().SetAbilitySlotInput(binding.slot, pressed);
			}
			if (GetPlayer().GetMovementComponent().GetMovementMode() == ShipMovementMode::LegacyVelocity)
			{
				NormalizeInput();
			}
			return;
		}

		// Device state is global: never read it for a headless world or while the
		// window is unfocused, otherwise typing elsewhere drives the ship.
		const World* world = GetPlayer().GetWorld();
		const bool hasInputFocus = world && world->HasInputFocus();
		const auto isKeyPressed = [hasInputFocus](sf::Keyboard::Key key)
		{
			return hasInputFocus && sf::Keyboard::isKeyPressed(key);
		};

		mMoveInput = { 0.f, 0.f };
		if (isKeyPressed(sf::Keyboard::Key::W) || isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;
		}
		const bool flightControlEnabled = !GetPlayer().GetModuleApplier().HasApplied() ||
			GetPlayer().GetModuleApplier().IsSystemEnabled(GameplayTags::Module::System::FlightControl);
		if (flightControlEnabled)
		{
			if (mMoveInput.y == 0.f &&
				(isKeyPressed(sf::Keyboard::Key::S) || isKeyPressed(sf::Keyboard::Key::Down)))
			{
				mMoveInput.y = 1.f;
			}

			if (isKeyPressed(sf::Keyboard::Key::A) || isKeyPressed(sf::Keyboard::Key::Left))
			{
				mMoveInput.x = -1.f;
			}
			else if (isKeyPressed(sf::Keyboard::Key::D) || isKeyPressed(sf::Keyboard::Key::Right))
			{
				mMoveInput.x = 1.f;
			}
		}

		if (mUseScreenClamp)
		{
			ClampInputOnEdge();
		}

		mAfterburnerRequested =
			isKeyPressed(sf::Keyboard::Key::LShift) ||
			isKeyPressed(sf::Keyboard::Key::RShift);

		GetPlayer().GetMovementComponent().SetAbilityWorldMovementInput(
			ResolveAbilityWorldMovementInput()
		);

		CombatRuntime& combatRuntime = GetPlayer().GetCombatRuntime();
		for (const AbilityInputBinding& binding : AbilityInputSchema::GetBindings())
		{
			combatRuntime.GetAbilitySystemComponent().SetAbilitySlotInput(
				binding.slot,
				isKeyPressed(binding.key)
			);
		}

		if (GetPlayer().GetMovementComponent().GetMovementMode() == ShipMovementMode::LegacyVelocity)
		{
			NormalizeInput();
		}
	}

	void PlayerMovementComponent::ConsumeInput(float deltaTime)
	{
		MovementComponent& movement = GetPlayer().GetMovementComponent();
		if (GetPlayer().GetAbilitySystemComponent().HasOwnedTag(
			GameplayTags::State::ActionLock::MovementInput
		))
		{
			// Focus rejects only new player translation. It must not overwrite the
			// velocity that existed before the lock, while mouse aiming stays live.
			mMoveInput = { 0.f, 0.f };
			mSmoothedMoveInput = { 0.f, 0.f };
			mAfterburnerRequested = false;
			mAfterburnerIsActive = false;
			mAfterburnerIntensity = 0.f;
			RotateTowardMouseCursor(deltaTime);
			return;
		}
		UpdateAfterburnerState(deltaTime);
		if (movement.IsMovementBurstActive())
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
			if (mAfterburnerIsActive && !hasMovementInput)
			{
				forwardInput = 1.f;
			}

			const float forwardThrust = forwardInput >= 0.f
				? movement.ResolveForwardThrust()
				: movement.ResolveReverseThrust();
			const float strafeThrust = movement.ResolveStrafeThrust();
			sf::Vector2f worldAcceleration =
				GetPlayer().GetActorForwardDirection() * forwardInput * forwardThrust +
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
			worldAcceleration *= GetPlayer().GetConditionalMovementSpeedMultiplier(
				worldAcceleration
			);

			const float afterburnerAccelerationMultiplier = std::max(
				1.f,
				GetPlayer().GetShipRuntime().GetAfterburnerAccelerationMultiplier()
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
				movementDirection = GetPlayer().GetActorForwardDirection();
			}

			const float afterburnerSpeedMultiplier = std::max(
				1.f,
				GetPlayer().GetShipRuntime().GetAfterburnerSpeedMultiplier()
			);
			const sf::Vector2f normalSpeed =
				movement.ResolveLegacySpeed() *
				GetPlayer().GetMovementSpeedMultiplier() *
				GetPlayer().GetConditionalMovementSpeedMultiplier(movementDirection);
			const float speedMultiplier = Lerp(1.f, afterburnerSpeedMultiplier, mAfterburnerIntensity);
			const sf::Vector2f targetVelocity{
				movementDirection.x * normalSpeed.x * speedMultiplier,
				movementDirection.y * normalSpeed.y * speedMultiplier
			};

			if (IsAfterburning())
			{
				const float accelerationMultiplier = std::max(
					1.f,
					GetPlayer().GetShipRuntime().GetAfterburnerAccelerationMultiplier()
				);
				const float responseRate = std::max(
					0.f,
					movement.GetAttributes().inputResponsiveness.currentValue * accelerationMultiplier
				);
				const float responseAlpha = responseRate > 0.f
					? 1.f - std::exp(-responseRate * deltaTime)
					: 1.f;
				GetPlayer().SetVelocity(LerpVector(GetPlayer().GetVelocity(), targetVelocity, responseAlpha));
			}
			else
			{
				GetPlayer().SetVelocity(targetVelocity);
			}
		}

		mMoveInput = { 0.f, 0.f };
	}

	void PlayerMovementComponent::UpdateAfterburnerState(float deltaTime)
	{
		mAfterburnerIsActive = false;
		if (mAfterburnerRequested && deltaTime > 0.f &&
			GetPlayer().GetShipRuntime().IsSystemEnabled(ShipSystem::Afterburner))
		{
			const float baseDrainPerSecond = std::max(
				0.f,
				GetPlayer().GetShipRuntime().GetAfterburnerEnergyDrainPerSecond()
			);
			const float drainPerSecond = baseDrainPerSecond * std::max(
				0.f,
				GetPlayer().GetRuntimeModifiers().GetAfterburnerEnergyDrainMultiplier()
			);
			const float requestedEnergy = drainPerSecond * deltaTime;
			if (requestedEnergy > 0.f)
			{
				mAfterburnerIsActive = GetPlayer().GetEnergyComponent().Consume(requestedEnergy) > 0.f;
			}
			else
			{
				// Free afterburner modifiers suppress resource consumption without
				// suppressing the actual afterburner movement state.
				mAfterburnerIsActive = true;
			}
		}

		const float targetIntensity = mAfterburnerIsActive ? 1.f : 0.f;
		const float transitionDuration = targetIntensity > mAfterburnerIntensity
			? std::max(0.f, GetPlayer().GetShipRuntime().GetAfterburnerRampUpDuration())
			: std::max(0.f, GetPlayer().GetShipRuntime().GetAfterburnerRampDownDuration());
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
		if (GetPlayer().GetActorLocation().x - 40.f <= 0.f && mMoveInput.x == -1.f)
		{
			mMoveInput.x = 0.f;
		}
		else if (GetPlayer().GetActorLocation().x + 40.f >= GetPlayer().GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;
		}

		if (GetPlayer().GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;
		}
		else if (GetPlayer().GetActorLocation().y + 40.f >= GetPlayer().GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f;
		}
	}

	sf::Vector2f PlayerMovementComponent::GetAdaptiveScreenStrafeDirection() const
	{
		const sf::Vector2f screenRight{ 1.f, 0.f };
		const sf::Vector2f horizontalFacingStrafe{ 0.f, -1.f };
		const sf::Vector2f shipForward = GetPlayer().GetActorForwardDirection();
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
		if (GetPlayer().GetMovementComponent().GetMovementMode() != ShipMovementMode::ThrustDrift)
		{
			return mMoveInput;
		}

		const float forwardInput = std::clamp(-mMoveInput.y, -1.f, 1.f);
		const float strafeInput = std::clamp(mMoveInput.x, -1.f, 1.f);
		return
			GetPlayer().GetActorForwardDirection() * forwardInput +
			GetAdaptiveScreenStrafeDirection() * strafeInput;
	}

	void PlayerMovementComponent::RotateTowardMouseCursor(float deltaTime)
	{
		if (World* world = GetPlayer().GetWorld())
		{
			GetPlayer().GetMovementComponent().RotateTowardWorldLocation(
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
			GetPlayer().GetShipRuntime().GetAfterburnerSpeedMultiplier()
		);
		return Lerp(1.f, afterburnerSpeedMultiplier, mAfterburnerIntensity);
	}

	float PlayerMovementComponent::GetMovementTurnCapabilityMultiplier() const
	{
		const float maneuverabilityMultiplier = std::clamp(
			GetPlayer().GetShipRuntime().GetAfterburnerManeuverabilityMultiplier(),
			0.f,
			1.f
		);
		return Lerp(1.f, maneuverabilityMultiplier, mAfterburnerIntensity) *
			GetPlayer().GetRuntimeModifiers().GetTurnCapabilityMultiplier();
	}

	bool PlayerMovementComponent::IsAfterburnerRechargeBlocked() const
	{
		return IsAfterburning();
	}
}
