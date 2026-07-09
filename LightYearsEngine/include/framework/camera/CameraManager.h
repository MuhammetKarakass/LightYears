#pragma once

#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <optional>

namespace ly
{
	class Actor;

	struct CameraSettings
	{
		float baseZoom = 1.55f;               // Default camera distance; higher values show more arena space.
		float lookAheadStrength = 0.45f;      // How strongly the camera moves from the ship toward the cursor, from 0 to 1.
		float lookAheadDeadZone = 380.f;      // Cursor distance from the ship before camera look-ahead starts reacting.
		float lookAheadMaxDistance = 600.f;   // Maximum cursor distance used for look-ahead so the camera cannot drift too far.
		float lookAheadSmoothingSpeed = 1.65f; // Cursor look-ahead follow speed; lower values make mouse camera movement slower.
		float movementLookAheadStrength = 0.75f; // How strongly ship velocity pulls the camera toward the movement direction.
		float movementLookAheadMaxDistance = 320.f; // Maximum movement-direction camera offset at high ship speed.
		float combinedLookAheadMaxOffset = 380.f; // Final clamp for cursor + movement look-ahead so they cannot stack too far.
		float speedForMaxZoomOut = 500.f;     // Ship speed that reaches the full speed-based zoom-out amount.
		float maxSpeedZoomOut = 0.32f;        // Extra zoom added at high speed on top of baseZoom.
		float positionSmoothingSpeed = 2.75f; // Camera center follow speed; lower values feel smoother and heavier.
		float zoomSmoothingSpeed = 3.2f;      // Camera zoom follow speed; lower values make zoom changes softer.
		float worldBoundsPadding = 200.f;     // Extra world-space area the camera may show beyond the configured bounds.
		bool useSmoothSpeedZoom = true;       // Uses smoothstep for speed zoom instead of a direct linear response.
	};

	class CameraManager
	{
	public:
		void SetSettings(const CameraSettings& settings);
		const CameraSettings& GetSettings() const { return mSettings; }

		void SetFollowTarget(weak_ptr<Actor> target);
		void ClearFollowTarget();

		void SetExternalVelocity(const sf::Vector2f& velocity);
		void ClearExternalVelocity();

		void SetLookAheadWorldPosition(const std::optional<sf::Vector2f>& worldPosition);
		void ClearLookAheadWorldPosition();
		void SetWorldBounds(const sf::FloatRect& bounds);
		void ClearWorldBounds();

		void Update(float deltaTime, const sf::View& defaultView);
		sf::View GetView(const sf::View& defaultView) const;

	private:
		sf::Vector2f GetDesiredCenter(const Actor& followTarget, float deltaTime);
		sf::Vector2f ClampCenterToWorldBounds(const sf::Vector2f& center, const sf::Vector2f& viewSize) const;
		float GetDesiredZoom() const;

		CameraSettings mSettings{};
		weak_ptr<Actor> mFollowTarget{};
		sf::Vector2f mExternalVelocity{};
		std::optional<sf::Vector2f> mLookAheadWorldPosition{};
		std::optional<sf::FloatRect> mWorldBounds{};

		bool mHasState = false;
		sf::Vector2f mCurrentCenter{};
		sf::Vector2f mCurrentLookAheadOffset{};
		float mCurrentZoom = 1.f;
	};
}
