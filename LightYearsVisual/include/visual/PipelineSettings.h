#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <SFML/System/Vector3.hpp>

#include "visual/LuminanceTier.h"
#include "visual/Nebula.h"

namespace ly::visual
{
	// The backdrop planet (drawn by the game into Background beneath the nebula). Positions are fractions of the output
	// size at the camera start; the planet then drifts against camera motion by `parallax`.
	struct PlanetSettings
	{
		bool enabled = true;
		float screenX = 0.95f;
		float screenY = 0.14f;
		// fraction of the output height
		float radius = 0.15f;
		// output pixels of drift per output pixel of camera motion (0 = fixed to the screen)
		float parallax = 0.025f;
		float brightness = 0.45f;
		float saturation = 0.55f;
		float opacity = 1.f;
		float rotationSpeed = 0.03f;
		float dither = 0.3f;
	};

	// The host star of the current system, drawn under the nebula like the planet. Positions are fractions of the output
	// size at the camera start, drifting by `parallax` like the planet.
	struct HostStarSettings
	{
		bool enabled = true;
		float screenX = 0.82f;
		float screenY = 0.08f;
		float radiusAt1AU = 0.012f;    // fraction of the output height
		float glowRadius = 0.12f;      // fraction of the output height
		// Disc brightness at 1 AU is intensity * (1 - exp(-1.5)) = 0.78 of the cap at 1.0 (0.70 at 0.9, too dim).
		float intensity = 1.f;
		float parallax = 0.006f;
	};

	// Star counts follow a disk population: density scales the count and shifts magnitudes, placement follows the band.
	struct StarFieldSettings
	{
		int baseCount = 900;              // stars at the Sun's stellar density
		// Budget cap toward the galactic centre: the bulge is a dense star field, so it is resolved into stars, not fog.
		int maxCount = 4000;
		float brightestMagnitude = -1.5f;
		float faintestMagnitude = 6.5f;
		float magnitudeSlope = 0.5f;      // star counts grow 10^slope per magnitude fainter
		float minLevel = 0.35f;           // display level of a faintestMagnitude star, fraction of the tier cap
		float levelGamma = 1.0f;
		float spikeMagnitude = 0.5f;      // brighter stars get 1-px diffraction spikes
		float variableFraction = 0.04f;
		float bandFraction = 0.5f;        // share of stars concentrated around the Milky Way band
		float nearFraction = 0.06f;       // share of stars on the nearer parallax layer
		float farParallax = 0.02f;        // output px of drift per output px of camera motion
		float nearParallax = 0.12f;
	};

	struct MilkyWaySettings
	{
		bool enabled = true;
		float angleDeg = 28.f;            // band direction on screen; the +x end faces the galactic centre
		float offset = 0.1f;              // band centre offset from the screen centre, perpendicular, fraction of height
		float width = 0.22f;              // gaussian half width, fraction of height
		float intensity = 0.9f;           // band brightness at the Sun, fraction of the background tier cap
		float dustLane = 0.6f;            // 0..1 depth of the central dust lane
		float parallax = 0.01f;
		float glowIntensity = 0.15f;      // skyGlow -> whole-screen diffuse light, fraction of the cap (low: stars, not fog)
	};

	// Anomalies of the backdrop: a pulsar, a shattered moon and the lens of Sgr A*. Chances are per location, seeded.
	struct AnomalySettings
	{
		float pulsarChance = 0.15f;
		float shatteredMoonChance = 0.2f;
		float cometChance = 0.f;          // a comet streaking across the stop's sky (off: it read as a shooting star)
		float lensEinsteinRadius = 0.16f; // fraction of the output height
		float lensShadowRadius = 0.04f;   // fraction of the output height
		float lensRingBrightness = 0.25f; // fraction of the background cap
	};

	// Ship speed for the star-field aberration. beta is clamped to maxBeta; the ramp eases toward the target.
	struct WarpSettings
	{
		float maxBeta = 0.92f;
		float rampSeconds = 1.2f;
		// Angular span of the screen across the motion, radians. Wide on purpose: at 1.2 rad a beta of 0.85 aberrates
		// every star off the top of the screen. At 3.0 the screen covers almost the whole sky, from behind to ahead, and
		// the stars crowd forward instead of vanishing.
		float fieldOfViewRad = 3.0f;
	};

	// Colour grade of the composited background (bg_grade.frag). Works in units of the background tier cap, so the
	// grade never lifts the background above it. Tints are colour multipliers; their luma is normalized away.
	struct BackgroundGradeSettings
	{
		bool enabled = true;
		float contrast = 1.45f;                          // S-curve strength around mid-grey of the cap, 1 = none
		float saturation = 1.35f;                        // chroma boost, luma-preserving
		float blackPoint = 0.015f;                       // output values below this crush to black
		sf::Vector3f shadowTint{0.55f, 0.45f, 1.f};      // shadows lean cool violet-blue
		float shadowTintStrength = 0.22f;
		sf::Vector3f highlightTint{1.f, 0.88f, 0.72f};   // highlights lean warm
		float highlightTintStrength = 0.12f;
	};

	// Stylized nebula composite (bg_stylize.frag): the nebula is drawn on a pixel grid, cut into alpha tone bands and its
	// colour snapped to a palette built from `roles`. pixelSize 0 uses the world pixel scale.
	struct BackgroundStyleSettings
	{
		bool enabled = false;
		float pixelSize = 0.f;
		int toneLevels = 5;
		float ditherSpread = 0.12f;
		std::vector<std::string> roles{"background", "emissionRed", "emissionTeal", "galaxyLight", "bulgeLight", "ambient", "ambient2"};
	};

	// Near-field rocks and motes drawn over the background (ProceduralBackdrop, gallery). Off by default.
	struct NearFieldSettings
	{
		bool enabled = false;
	};

	// Lit soft cloud masses in three parallax layers (skypaint.frag), coloured by the region's sky palette.
	struct SkyPaintSettings
	{
		bool enabled = true;
		float coverage = 0.4f;            // 0..1 how much of the sky the clouds fill
		float intensity = 1.f;
		float centerCalm = 0.55f;         // 0..1 dims mid and near detail around the screen centre
		float farParallax = 0.015f;       // output px of drift per output px of camera motion
		float midParallax = 0.035f;
		float nearParallax = 0.07f;
	};

	bool operator==(const BackgroundStyleSettings& a, const BackgroundStyleSettings& b);
	bool operator!=(const BackgroundStyleSettings& a, const BackgroundStyleSettings& b);
	bool operator==(const NearFieldSettings& a, const NearFieldSettings& b);
	bool operator!=(const NearFieldSettings& a, const NearFieldSettings& b);
	bool operator==(const BackgroundGradeSettings& a, const BackgroundGradeSettings& b);
	bool operator!=(const BackgroundGradeSettings& a, const BackgroundGradeSettings& b);
	bool operator==(const NebulaSettings& a, const NebulaSettings& b);
	bool operator!=(const NebulaSettings& a, const NebulaSettings& b);
	bool operator==(const AnomalySettings& a, const AnomalySettings& b);
	bool operator!=(const AnomalySettings& a, const AnomalySettings& b);
	bool operator==(const WarpSettings& a, const WarpSettings& b);
	bool operator!=(const WarpSettings& a, const WarpSettings& b);
	bool operator==(const HostStarSettings& a, const HostStarSettings& b);
	bool operator!=(const HostStarSettings& a, const HostStarSettings& b);
	bool operator==(const StarFieldSettings& a, const StarFieldSettings& b);
	bool operator!=(const StarFieldSettings& a, const StarFieldSettings& b);
	bool operator==(const MilkyWaySettings& a, const MilkyWaySettings& b);
	bool operator!=(const MilkyWaySettings& a, const MilkyWaySettings& b);
	bool operator==(const SkyPaintSettings& a, const SkyPaintSettings& b);
	bool operator!=(const SkyPaintSettings& a, const SkyPaintSettings& b);

	// Every runtime knob of the render pipeline. Loaded from pipeline.json, editable while running.
	struct PipelineSettings
	{
		int virtualHeight = 240;
		int nebulaResolutionDivisor = 1;
		bool ditherEnabled = true;
		float ditherSpread = 0.1f;

		bool bloomEnabled = true;
		float bloomThreshold = 0.55f;
		float bloomStrength = 0.9f;
		int bloomLevels = 3;

		float vignetteStrength = 0.4f;
		float vignetteRadius = 0.7f;
		float flashScale = 1.f;

		PlanetSettings planet;
		HostStarSettings hostStar;
		StarFieldSettings starField;
		MilkyWaySettings milkyWay;
		SkyPaintSettings skyPaint;
		// Emission and dust clouds of the backdrop (Nebula.h); the Milky Way band stays in milkyWay.
		NebulaSettings physicalNebula;
		AnomalySettings anomalies;
		WarpSettings warp;
		BackgroundGradeSettings backgroundGrade;
		BackgroundStyleSettings backgroundStyle;
		NearFieldSettings nearField;

		int maxEnergyDraws = 96;
		float maxEnergyAreaRatio = 1.5f;
		std::array<float, LuminanceTierCount> tierMaxBrightness = {0.35f, 0.5f, 0.65f, 0.85f, 1.f};
	};

	// Starts from defaults, applies the file, validates. On failure out is left untouched.
	bool LoadPipelineSettings(const std::filesystem::path& file, PipelineSettings& out, std::string& error);
}
