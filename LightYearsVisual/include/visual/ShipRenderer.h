#pragma once

#include <array>

#include <SFML/Graphics.hpp>

#include "visual/ShapeBrush.h"
#include "visual/ShipVisual.h"
#include "visual/VisualAssets.h"
#include "visual/VisualPipeline.h"

namespace ly::visual
{
	// One ability port as the caller sees it this frame.
	struct ShipPortState
	{
		bool equipped = false;
		AbilityCategory category = AbilityCategory::Utility;
		// 0 = the cooldown just started, 1 = finished.
		float cooldownFraction = 1.f;
		bool isReady = true;
		bool isActive = false;
		// Counts down 1 -> 0 after an activation (the caller picks the duration, about 0.4 s).
		float justActivated = 0.f;
	};

	// Everything the renderer needs for one ship and one frame. Plain data: the caller fills it, nothing is retained.
	struct ShipVisualState
	{
		// Output pixels (layer coordinates of the pipeline), nose direction rotated clockwise from up.
		sf::Vector2f position;
		float rotationDegrees = 0.f;
		// Output pixels per local ship unit; the definition's own scale multiplies it.
		float scale = 1.f;
		float hullFraction = 1.f;
		float shieldFraction = 0.f;
		// 1 -> 0 timers after the hull / the shield took a hit.
		float hitFlash = 0.f;
		float shieldHit = 0.f;
		// Local angle (radians, 0 = ship's right side) where the last shield hit lands.
		float shieldHitAngle = 0.f;
		float thrust = 0.f;
		bool afterburner = false;
		// Spawn protection and similar: the hull flickers.
		bool ghost = false;
		std::array<ShipPortState, ShipPortCount> ports;
		// Monotonic seconds; drives every animation so equal times draw equal frames.
		float time = 0.f;
		// Stable per ship instance; seeds crack and smoke layouts together with the part seeds.
		unsigned seed = 1;
	};

	// Draws a data-driven ship: hull in the dithered World layer, lights, shield and ability ports as energy.
	class ShipRenderer
	{
	public:
		explicit ShipRenderer(VisualAssets& assets);

		void Draw(VisualPipeline& pipeline, const ShipVisualDefinition& definition, const ShipVisualState& state);

		// 0 clean (>75%), 1 cracks (>50%), 2 sparks and a missing plate (>25%), 3 heavy damage.
		static int DamageStage(float hullFraction);

	private:
		VisualAssets& mAssets;
		ShapeBrush mBrush;
	};
}
