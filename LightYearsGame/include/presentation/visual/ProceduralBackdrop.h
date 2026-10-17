#pragma once

#include <SFML/Graphics.hpp>
#include <visual/Anomalies.h>
#include <visual/Backdrop.h>
#include <visual/DeepSky.h>
#include <visual/Galaxy.h>
#include <visual/GalaxyModel.h>
#include <visual/NearField.h>
#include <visual/PipelineSettings.h>
#include <visual/VisualAssets.h>
#include <visual/VisualPipeline.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vega
{
	class World;
	struct RouteStop;

	// Procedural space backdrop for the place set by SetRouteStop. Back to front: gradient, host star, backdrop planet,
	// shattered moon debris, Milky Way band, emission nebula (both in the nebula target, composited), dust cloud, the
	// blackbody star field, pulsar, the near-field rocks and motes, and the Sgr A* lens over the whole background. The host star and the planet sit
	// behind the band and the nebula on purpose (the user's art direction); the stars are drawn on top. Which features a
	// place gets is seeded from the stop id, so the same stop always looks the same. Tuned by the "planet", "hostStar",
	// "physicalNebula", "anomalies", "warp" and "milkyWay" sections of pipeline.json, and per stop by deepsky.json. It only reads the camera view and
	// never touches gameplay state. An orbit stop (deepsky.json "orbit") replaces the composition with a horizon or disk planet and a
	// rigidly turning sky.
	class ProceduralBackdrop
	{
	public:
		ProceduralBackdrop();

		// Call after VisualPipeline::BeginFrame with the continuous game view for smooth background parallax.
		// The first view seen for a world is its camera start: the planet sits at its configured screen position
		// there and drifts relative to it.
		void Draw(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const World& world, const sf::View& worldView, float timeSeconds);

		// The place on the campaign route: its position, host star, planets (tint and moon) and a seed from the stop id.
		void SetRouteStop(const RouteStop& stop);
		// A bare position without a route stop: sun-like host star, no planets, seed from the position.
		void SetGalacticPosition(const visual::galaxy::GalacticPosition& position);
		// Ship speed as a fraction of light speed and its on-screen direction (y down). Eases toward targetBeta over
		// warp.rampSeconds and is clamped to warp.maxBeta; a zero direction keeps the previous one.
		void SetWarp(float targetBeta, sf::Vector2f direction);

		const std::string& StopId() const { return mStopId; }
		const visual::galaxy::SkyState& Sky() const { return mSky; }

		// Dev-only: steps the orbit speed multiplier of orbit stops (+1 faster, -1 slower) through a fixed list, 0 pauses.
		void StepOrbitSpeed(int step);
		float OrbitSpeedScale() const;
		// Degrees per minute the sky turned at in the last drawn frame, scale included; empty when the stop is not an orbit.
		std::optional<float> OrbitDegreesPerMinute() const { return mOrbitRate; }
		struct SkyGeometry
		{
			sf::Vector2f planetAt;
			sf::Vector2f bandDrift;
			float planetRadius = 0.f;
			float planetBaseScale = 1.f;
			visual::MilkyWaySettings band;
			bool mirrored = false, hasPlanet = false;
		};
		// Read the same composition used by Draw; authoring handles must not approximate seeded placement.
		SkyGeometry EditorGeometry(const visual::VisualAssets& assets, sf::Vector2u output) const;

	private:
		// Emission and dust clouds and the pulsar, positioned as fractions of the output size.
		struct EmissionState
		{
			bool present = false;
			sf::Vector2f at;
			float temperatureK = 38000.f;
			float seed = 0.f;
		};

		struct DustState
		{
			bool present = false;
			sf::Vector2f at;
			sf::Vector2f illuminatorAt;
			float angleRad = 0.f;
			float seed = 0.f;
		};

		struct PulsarState
		{
			bool present = false;
			sf::Vector2f at;
			float periodSeconds = 1.337f;
		};

		void RebuildFeatures(const visual::PipelineSettings& settings);
		void UpdateWarp(const visual::WarpSettings& warp, float timeSeconds);
		void DrawBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds);
		void DrawHostStar(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float maxBrightness);
		// The host star disc at a screen point, with its radius, glow and colour from the host star; a radius in output pixels overrides the default.
		void DrawHostStarAt(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f center, float maxBrightness, std::optional<float> radius);
		bool DrawPlanet(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds);
		// The low-orbit stop: a horizon or disk planet (with its ring and moon), rigidly turning sky and stars, host star and motes; no objects or nebula.
		void DrawOrbitBackground(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const visual::DeepSkyStop& deepSky, const visual::OrbitSettings& orbit, sf::Vector2f cameraCenter, float viewHeight, float timeSeconds, float cap);
		// Rebuilds the field stars when the sky, the band, the stop's clusters or the catalog changed; nullptr adds no clusters.
		void RebuildStars(const visual::StarFieldSettings& starField, const visual::MilkyWaySettings& band, const visual::DeepSkyStop* clusters, bool mirrored, sf::Vector2f outputSize, unsigned catalogVersion);
		// Near-field dust motes, lit by the star direction and thinned or thickened by the stop.
		void DrawMotes(visual::VisualAssets& assets, visual::VisualPipeline& pipeline, const visual::DeepSkyStop* deepSky, sf::Vector2f scrollPx, float timeSeconds, float cap);

		visual::StarField mStars;
		visual::galaxy::GalacticPosition mPosition;
		visual::galaxy::SkyState mSky;
		visual::HostStar mHostStar;
		// Kind of each planet (palette, bands, clouds), from its own data; mPlanetKind is the first one, the backdrop planet.
		std::vector<visual::PlanetKind> mPlanetKinds;
		visual::PlanetKind mPlanetKind = visual::PlanetKind::Terran;
		std::size_t mPlanetCount = 0;
		// False for a route stop without planets: no backdrop planet (and so no moon) is drawn there.
		bool mHasPlanet = true;
		// Procedural stops (and bare positions) may carry a pulsar; real catalogue hosts never do.
		bool mProcedural = true;
		// Progress along the route, nearest sample; pulsars need more than 0.2 of it.
		float mRouteProgress = 0.f;
		// Sgr A* is the final stop: the lens is on only there.
		bool mLensLocation = false;
		std::uint64_t mLocationSeed = 0;

		EmissionState mEmission;
		DustState mDust;
		PulsarState mPulsar;
		std::optional<visual::ShatteredMoon> mMoon;
		// Near-field rocks and motes, rebuilt with the location seed.
		std::optional<visual::NearField> mNearField;
		visual::GalaxyLayer mGalaxy;
		// Per-stop nebulae, haze, clusters and galaxies from deepsky.json, keyed by the route stop id.
		visual::DeepSkyLayer mDeepSky;
		// The low-orbit panorama of the sky, baked per location (its sky and tint are not cache keys).
		visual::OrbitSky mOrbit;
		// The stop's asteroid belt, rebuilt when the stop or its settings change.
		visual::AsteroidField mAsteroids;
		std::string mAsteroidsStop;
		visual::AsteroidBeltSettings mAsteroidsBuilt;
		// Empty for a bare galactic position (no deep-sky entry applies).
		std::string mStopId;
		// The deep-sky catalog version and stop the star field's clusters were built for.
		unsigned mBuiltStarsCatalog = 0;
		std::string mBuiltStarsStop;

		// Set by the location setters; the next Draw starts the location clock and rebuilds the features.
		bool mLocationChanged = true;
		bool mFeaturesDirty = true;
		float mLocationStart = 0.f;
		// Orbit clock: seconds of orbit since the location start, advanced at the speed multiplier, so a speed change never jumps the sky.
		float mOrbitSeconds = 0.f;
		float mOrbitClockTime = 0.f;
		std::size_t mOrbitSpeedStep = 3;
		std::optional<float> mOrbitRate;
		visual::NebulaSettings mBuiltNebula;
		visual::AnomalySettings mBuiltAnomalies;

		// Warp: beta eases toward mTargetBeta; mDirection is the unit on-screen direction of motion.
		float mTargetBeta = 0.f;
		float mBeta = 0.f;
		float mLastTime = 0.f;
		sf::Vector2f mDirection{0.f, -1.f};

		// Output-pixel geometry of the planet from the last draw, for the moon and the pulsar.
		sf::Vector2f mPlanetCenter;
		float mPlanetRadius = 0.f;
		sf::Vector2f mStarDirection{-0.85f, -0.5f};

		// The stars and the band are rebuilt when the sky or the settings they were built from change.
		bool mStarsDirty = true;
		visual::StarFieldSettings mBuiltStars;
		visual::MilkyWaySettings mBuiltMilkyWay;
		const World* mAnchoredWorld = nullptr;
		sf::Vector2f mAnchoredViewSize;
		sf::Vector2f mCameraStart;
		sf::Vector2f mLastCameraCenter;
	};
}
