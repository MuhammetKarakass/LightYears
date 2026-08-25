#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class PlayerSpaceShip;

	// Translates local player controls into movement requests for a ship.
	class PlayerMovementComponent
	{
	public:
		explicit PlayerMovementComponent(PlayerSpaceShip& owner);

		void Tick(float deltaTime);

		void SetSpeed(float speed);
		float GetSpeed() const;
		void SetUseScreenClamp(bool useClamp) { mUseScreenClamp = useClamp; }
		bool GetUseScreenClamp() const { return mUseScreenClamp; }
		bool IsAfterburning() const { return mAfterburnerIntensity > 0.001f; }
		float GetAfterburnerCameraZoomOut() const { return mAfterburnerIntensity * 0.18f; }

		float GetMovementSpeedCapMultiplier() const;
		float GetMovementTurnCapabilityMultiplier() const;
		bool IsAfterburnerRechargeBlocked() const;

	private:
		void SetInput();
		void ConsumeInput(float deltaTime);
		void UpdateAfterburnerState(float deltaTime);
		void NormalizeInput();
		void ClampInputOnEdge();
		sf::Vector2f GetAdaptiveScreenStrafeDirection() const;
		sf::Vector2f ResolveAbilityWorldMovementInput() const;
		void RotateTowardMouseCursor(float deltaTime);

		PlayerSpaceShip& mOwner;
		sf::Vector2f mMoveInput{ 0.f, 0.f };
		sf::Vector2f mSmoothedMoveInput{ 0.f, 0.f };
		float mAfterburnerIntensity = 0.f;
		bool mUseScreenClamp = true;
		bool mAfterburnerRequested = false;
		bool mAfterburnerIsActive = false;
	};
}
