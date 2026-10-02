#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/Backdrop.h"
#include "visual/ShapeBrush.h"
#include "visual/VisualAssets.h"
#include "visual/VisualPipeline.h"

// Sample scene that exercises every V1 piece: backdrop, planet, polygon ship, SDF shapes, glows of every tier.
class GalleryScene
{
public:
	GalleryScene(ly::visual::VisualAssets& assets, ly::visual::VisualPipeline& pipeline);

	void Update(float deltaSeconds);
	// Call between pipeline.BeginFrame() and pipeline.Compose().
	void Draw();
	void ToggleStress() { mStress = !mStress; }

private:
	void DrawBackground();
	void DrawWorld();
	void DrawEnergy();
	sf::Vector2f ShipToScene(sf::Vector2f local) const;
	std::vector<sf::Vector2f> ShipToScene(const std::vector<sf::Vector2f>& local) const;

	ly::visual::VisualAssets& mAssets;
	ly::visual::VisualPipeline& mPipeline;
	ly::visual::ShapeBrush mBrush;
	ly::visual::StarField mStars;
	float mTime = 0.f;
	sf::Vector2f mScroll;
	bool mStress = false;
};
