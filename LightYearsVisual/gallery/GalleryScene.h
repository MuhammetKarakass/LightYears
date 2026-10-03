#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/Backdrop.h"
#include "visual/ShapeBrush.h"
#include "visual/ShipRenderer.h"
#include "visual/VisualAssets.h"
#include "visual/VisualPipeline.h"

// Sample scene: backdrop, planet, every ship definition (V3) at full and game size, damage stages, port states, glows of every tier.
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
	void DrawShips();
	void DrawEnergy();

	ly::visual::VisualAssets& mAssets;
	ly::visual::VisualPipeline& mPipeline;
	ly::visual::ShapeBrush mBrush;
	ly::visual::ShipRenderer mShips;
	ly::visual::StarField mStars;
	float mTime = 0.f;
	sf::Vector2f mScroll;
	bool mStress = false;
};
