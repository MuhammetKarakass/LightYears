#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/Anomalies.h"
#include "visual/Backdrop.h"
#include "visual/Galaxy.h"
#include "visual/GalaxyModel.h"
#include "visual/NearField.h"
#include "visual/Nebula.h"
#include "visual/ShapeBrush.h"
#include "visual/ShipRenderer.h"
#include "visual/VisualAssets.h"
#include "visual/VisualPipeline.h"

// Sample scene: backdrop (the same layers and order as the game's ProceduralBackdrop), planet, every ship definition (V3)
// at full and game size, damage stages, port states, glows of every tier.
class GalleryScene
{
public:
	GalleryScene(vega::visual::VisualAssets& assets, vega::visual::VisualPipeline& pipeline);

	void Update(float deltaSeconds);
	// Call between pipeline.BeginFrame() and pipeline.Compose().
	void Draw();
	void ToggleStress() { mStress = !mStress; }
	// Draws only the sky (no planet, ships or energy), for judging the backdrop on its own.
	void SetBackdropOnly(bool backdropOnly) { mBackdropOnly = backdropOnly; }
	bool BackdropOnly() const { return mBackdropOnly; }

	// Position along the route from the Sun (0) to Sgr A* (1); clamped.
	void SetGalaxyProgress(float progress);
	float GalaxyProgress() const { return mGalaxyProgress; }
	const vega::visual::galaxy::SkyState& Sky() const { return mSky; }

	// Ship speed as a fraction of light speed and its on-screen direction (y down). beta eases over warp.rampSeconds and
	// is clamped to warp.maxBeta; a zero direction keeps the previous one.
	void SetWarp(float targetBeta, sf::Vector2f direction);
	float WarpTarget() const { return mWarpTarget; }
	// Emission, dust, pulsar, moon and lens of the current place, or "none".
	std::string FeatureFlags() const;

private:
	// Per-place content, built with the same rules as the game's ProceduralBackdrop. Rebuilt when the progress or the
	// physical settings change; positions are fractions of the output.
	struct Location
	{
		vega::visual::HostStar hostStar;
		bool emission = false;
		sf::Vector2f emissionAt;
		float emissionTemperatureK = 38000.f;
		float emissionSeed = 0.f;
		bool dust = false;
		sf::Vector2f dustAt;
		sf::Vector2f dustIlluminatorAt;
		float dustAngleRad = 0.f;
		float dustSeed = 0.f;
		bool pulsar = false;
		sf::Vector2f pulsarAt;
		float pulsarPeriodSeconds = 1.337f;
		// Backdrop planet kind and orbiting moon seed, from the progress seed (the gallery has no route data).
		vega::visual::PlanetKind planetKind = vega::visual::PlanetKind::Terran;
		std::uint32_t moonSeed = 0;
		std::optional<vega::visual::ShatteredMoon> moon;
		bool lens = false;
	};

	void DrawBackground();
	void RebuildLocation(const vega::visual::PipelineSettings& settings);
	void DrawWorld();
	void DrawShips();
	void DrawEnergy();

	vega::visual::VisualAssets& mAssets;
	vega::visual::VisualPipeline& mPipeline;
	vega::visual::ShapeBrush mBrush;
	vega::visual::ShipRenderer mShips;
	vega::visual::StarField mStars;
	float mGalaxyProgress = 0.f;
	vega::visual::galaxy::GalacticPosition mPosition;
	vega::visual::galaxy::SkyState mSky;
	// The stars and the band are rebuilt when the sky or the settings they were built from change.
	bool mStarsDirty = true;
	vega::visual::StarFieldSettings mBuiltStars;
	vega::visual::MilkyWaySettings mBuiltMilkyWay;
	float mTime = 0.f;
	sf::Vector2f mScroll;
	bool mStress = false;
	bool mBackdropOnly = false;

	Location mLocation;
	// Near-field rocks and motes of the current place, rebuilt with it.
	std::optional<vega::visual::NearField> mNearField;
	vega::visual::GalaxyLayer mGalaxy;
	bool mFeaturesDirty = true;
	// Time the current place was set; the shattered moon's age counts from it.
	float mLocationStart = 0.f;
	vega::visual::NebulaSettings mBuiltNebula;
	vega::visual::AnomalySettings mBuiltAnomalies;

	// Warp: beta eases toward mWarpTarget; mWarpDirection is the unit on-screen direction of motion.
	float mWarpTarget = 0.f;
	float mBeta = 0.f;
	sf::Vector2f mWarpDirection{0.f, -1.f};
};
