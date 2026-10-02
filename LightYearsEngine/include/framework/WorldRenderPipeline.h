#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

namespace ly
{
	class World;
	enum class RenderLayer : uint8_t;

	// Optional presentation seam: when the application has an active pipeline, World::Render draws its actors
	// into the pipeline's layer targets instead of the window, then asks the pipeline to compose onto the window.
	// The HUD is always drawn to the window afterwards, so it is never filtered by the pipeline.
	// The engine knows only this interface; the game implements it (LightYearsVisual lives game-side).
	class WorldRenderPipeline
	{
	public:
		virtual ~WorldRenderPipeline() = default;

		// False renders the world straight to the window exactly as without a pipeline.
		virtual bool IsActive() const = 0;
		// Once per frame before any actor draws. worldView is the camera view the actors expect.
		virtual void BeginFrame(const World& world, const sf::View& worldView) = 0;
		virtual sf::RenderTarget& GetTarget(RenderLayer layer) = 0;
		// True when the pipeline paints this layer itself, so the world's actors on it are skipped.
		virtual bool ReplacesLayer(RenderLayer layer) const = 0;
		// After all actors drew. Leaves the window with the composed scene (default view), ready for the HUD.
		virtual void Compose(sf::RenderWindow& window) = 0;
	};
}
