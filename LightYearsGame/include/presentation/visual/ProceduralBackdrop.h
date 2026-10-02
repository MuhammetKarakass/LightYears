#pragma once

#include <SFML/Graphics.hpp>
#include <visual/Backdrop.h>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

namespace ly
{
	// Procedural space backdrop: nebula, parallax star field and one large planet, colored by the active color script.
	// It only reads the camera view; it never touches gameplay state.
	class ProceduralBackdrop
	{
	public:
		ProceduralBackdrop();

		// Call after VisualPipeline::BeginFrame and before the world view is applied to the World target.
		void Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const sf::View& worldView, float timeSeconds);

	private:
		void DrawBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float timeSeconds);
		void DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float timeSeconds);

		visual::StarField mStars;
	};
}
