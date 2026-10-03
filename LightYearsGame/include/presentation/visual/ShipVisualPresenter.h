#pragma once

#include "framework/Core.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <visual/ShipRenderer.h>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	class Actor;
	class PlayerSpaceShip;
	class World;

	// Draws the player's ship procedurally (LightYearsVisual ShipRenderer) from a read-only look at the ship: transform,
	// velocity, health and shield fractions, afterburner and the four ability slots. Hits are noticed by watching those
	// fractions drop, so no delegate is bound and nothing is written back to gameplay. Only plain per-frame trackers
	// are kept, keyed by the ship's unique id; the actor itself is re-queried every frame.
	// While it draws a ship, the pipeline asks HidesActor so the world skips that ship's sprite.
	class ShipVisualPresenter
	{
	public:
		// Call after VisualPipeline::BeginFrame with the pixel-snapped view. timeSeconds is a monotonic real-time clock.
		void Draw(const World& world, const sf::View& worldView, visual::VisualAssets& assets, visual::VisualPipeline& pipeline, float timeSeconds);
		// True for the player ship while this presenter drew (or deliberately hid) it this frame.
		bool HidesActor(const Actor& actor) const;
		// Dev-only: shows this hull fraction instead of the real one (screenshots). Gameplay health is untouched.
		void SetHullOverride(std::optional<float> hullFraction) { mHullOverride = hullFraction; }

	private:
		struct PortTracker
		{
			std::string abilityId;
			bool active = false;
			bool onCooldown = false;
			float burst = 0.f;
		};

		void Track(PlayerSpaceShip& ship, float deltaSeconds);

		std::unique_ptr<visual::ShipRenderer> mRenderer;
		std::optional<float> mHullOverride;
		unsigned int mHiddenShipId = 0;
		unsigned int mTrackedShipId = 0;
		float mLastTime = 0.f;
		float mLastHealth = 0.f;
		float mLastShield = 0.f;
		float mHullHit = 0.f;
		float mShieldHit = 0.f;
		int mShieldHits = 0;
		std::array<PortTracker, visual::ShipPortCount> mPorts;
	};
}
