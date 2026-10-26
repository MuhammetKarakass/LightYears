#pragma once

#include <framework/ActorComponent.h>
#include <SFML/System/Vector2.hpp>

#include <memory>

namespace vega
{
	class PlayerSpaceShip;
	class EnemyActor;

	// Translates local player controls into movement requests for a ship.
	// Not ticked automatically (mCanEverTick stays false): PlayerSpaceShip::Tick
	// runs it BEFORE SpaceShip::Tick/Actor::Tick, and automatic component ticking
	// happens inside Actor::Tick, so the owner drives it explicitly (UE
	// tick-prerequisite equivalent). The owner must be a PlayerSpaceShip.
	class PlayerMovementComponent : public ActorComponent
	{
		VEGA_CLASS(PlayerMovementComponent)
	public:
		explicit PlayerMovementComponent(Actor& owner);

		void Tick(float deltaTime);
		// Shared control boundary for device input and scripted/local controllers.
		void ApplyInput(const sf::Vector2f& localInput, bool afterburnerRequested, float deltaTime);
		void StopAfterburner();

		void SetSpeed(float speed);
		float GetSpeed() const;
		void SetUseScreenClamp(bool useClamp) { mUseScreenClamp = useClamp; }
		bool GetUseScreenClamp() const { return mUseScreenClamp; }
		bool IsAfterburning() const { return mAfterburnerIntensity > 0.001f; }
		float GetAfterburnerCameraZoomOut() const { return mAfterburnerIntensity * 0.18f; }

		float GetMovementSpeedCapMultiplier() const;
		float GetMovementTurnCapabilityMultiplier() const;
		bool IsAfterburnerRechargeBlocked() const;

		// Development only (--perf-autoplay): the ship circles, holds fire and taps every ability key, so performance runs
		// cover movement, weapons and abilities without a person at the keyboard.
		static void SetDevAutoPlay(bool enabled);

		// Possession: while a host is set, the controls steer the host instead of this ship (see PlayerSpaceShip).
		void SetPossessedHost(const std::weak_ptr<EnemyActor>& host) { mPossessedHost = host; }
		void ClearPossessedHost() { mPossessedHost.reset(); }
		bool HasPossessedHost() const { return !mPossessedHost.expired(); }
		std::shared_ptr<EnemyActor> GetPossessedHost() const { return mPossessedHost.lock(); }

	private:
		void SetInput();
		void ConsumeInput(float deltaTime);
		void UpdateAfterburnerState(float deltaTime);
		void NormalizeInput();
		void ClampInputOnEdge();
		sf::Vector2f GetAdaptiveScreenStrafeDirection() const;
		sf::Vector2f ResolveAbilityWorldMovementInput() const;
		void RotateTowardMouseCursor(float deltaTime);
		void SteerPossessedHost(EnemyActor& host);

		PlayerSpaceShip& GetPlayer() const;

		sf::Vector2f mMoveInput{ 0.f, 0.f };
		sf::Vector2f mSmoothedMoveInput{ 0.f, 0.f };
		float mAfterburnerIntensity = 0.f;
		bool mUseScreenClamp = true;
		bool mAfterburnerRequested = false;
		bool mAfterburnerIsActive = false;
		bool mAfterburnerWasRequested = false;
		bool mPrimaryFireHeld = false;
		std::weak_ptr<EnemyActor> mPossessedHost;
	};
}
