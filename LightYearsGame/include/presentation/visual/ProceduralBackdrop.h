#pragma once

#include <SFML/Graphics.hpp>
#include <visual/Backdrop.h>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	class World;

	// Procedural space backdrop: nebula, parallax star field and a distant planet, colored by the active color script.
	// The planet is tuned by the "planet" section of pipeline.json. It only reads the camera view; it never touches
	// gameplay state.
	class ProceduralBackdrop
	{
	public:
		ProceduralBackdrop();

		// Call after VisualPipeline::BeginFrame. worldView is the pixel-snapped view (VisualPipeline::SnapWorldView) so
		// parallax moves in lock step with the sprites. The first view seen for a world is its camera start: the planet
		// sits at its configured screen position there and drifts relative to it.
		void Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const World& world, const sf::View& worldView, float timeSeconds);

	private:
		void DrawBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float timeSeconds);
		void DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds);

		visual::StarField mStars;
		const World* mAnchoredWorld = nullptr;
		sf::Vector2f mCameraStart;
	};
}
