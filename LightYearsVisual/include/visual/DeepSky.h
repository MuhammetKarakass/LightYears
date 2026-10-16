#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "visual/AsteroidField.h"
#include "visual/Galaxy.h"
#include "visual/OrbitSky.h"
#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	enum class SkyFieldType { Number, Integer, Bool, Color, Position, Enum, Text };
	struct SkyField
	{
		std::string path; // Array elements use *, for example objects.*.size.
		SkyFieldType type;
		nlohmann::json defaultValue;
		std::optional<float> min, max;
		std::vector<std::string> options;
	};
	// Authoring metadata lives beside the parser; its numeric bounds are also used by parsing.
	const std::vector<SkyField>& DeepSkyFields();
	constexpr float SkyRingRadiusGap = 0.05f;
	const SkyField* FindSkyField(const std::string& path);
	std::string ValidateSkyField(const SkyField& field, const nlohmann::json& value);
	// Deep-sky objects of one route stop (deepsky.json): each stop can carry its own nebulae, haze, clusters and galaxies,
	// so the journey changes character from stop to stop. Every kind is a real phenomenon; colours come from its emission
	// lines or its light (H-alpha red/rose, [O III] teal, [S II] deep red, reflection blue, old-star gold).
	enum class DeepSkyKind
	{
		Emission,   // HII region: billowing gas, line colours, dark lanes; hollow = Rosette-like cavity
		Reflection, // dust lit by a hot star: soft blue, striated wisps
		Planetary,  // shell of a dying star: [O III] inside, H-alpha rim, knots; bipolar = butterfly lobes
		Remnant,    // supernova remnant: thin shock filaments, [O III] outside H-alpha; partial shell
		Bubble,     // Wolf-Rayet / LBV wind bubble: thin bright shell around a hot star
		Dark,       // cold dust: absorbs what is behind it, rim lit by nearby stars (Bok globules, pillars)
		Fog,        // broad, low-frequency coloured haze with dark rifts
		Cluster,    // globular (dense, gold) or open (sparse, blue, with haze)
		Galaxy,     // a distant galaxy: tilted disc, bulge, arms and dust
		Filaments,  // parallel synchrotron threads (the radio arc near the Galactic Centre)
		Lobes,      // a pair of huge faint outflow bubbles with edge brightening (Fermi / eROSITA bubbles)
		Jet,        // bipolar jet with knots (Herbig-Haro objects, microquasars)
		Spiral,     // the Sgr A West mini-spiral: ionized streamers falling toward the centre
		Belt,       // an asteroid belt: soft-lit rocks in a stream with a faint dust glow; occludes what is behind it
		Star        // a giant star close by: bright core, coloured halo and soft diffraction spikes (size = spike reach)
	};

	struct DeepSkyObject
	{
		DeepSkyKind kind = DeepSkyKind::Emission;
		sf::Vector2f at{0.5f, 0.5f}; // centre, fraction of the output at the camera start
		float size = 0.3f;           // radius, fraction of the output height
		float aspect = 1.f;          // y squash of the local frame (< 1 flattens)
		float angleDeg = 0.f;
		sf::Glsl::Vec3 colorA{1.f, 0.35f, 0.45f};
		sf::Glsl::Vec3 colorB{0.3f, 0.9f, 0.85f};
		sf::Glsl::Vec3 colorC{1.f, 0.9f, 0.8f};
		float intensity = 0.6f;      // fraction of the background cap
		float variant = 0.f;         // kind-specific switch (hollow, bipolar, open cluster, face-on ...)
		float seed = 0.f;
	};

	// Per-stop overrides of the Milky Way band, applied on top of the stop's composition.
	struct DeepSkyBand
	{
		std::optional<float> intensityScale;
		std::optional<float> angleDeg;
		std::optional<float> offset;
		std::optional<float> width;
		std::optional<float> dust;   // scales the band's dust lanes
		// Colours, gas clouds and viewing direction of the band ("view": "anticenter" / "outward" are shorthands).
		BandStyle style;
	};

	// Where the stop's backdrop planet sits and how big it is (for example dead centre, hiding part of the band).
	struct DeepSkyPlanet
	{
		std::optional<sf::Vector2f> at;   // screen fraction, authored for the unmirrored composition
		std::optional<float> scale;       // multiplies the planet radius
		bool hide = false;
	};

	// One layer of the soft, full-screen ramp-coloured clouds (nebula.frag) the game opened with: the ship flies inside
	// them. scale 1 is the original far layer; larger is closer (bigger shapes), parallax sets how fast they slide.
	struct DeepSkyNebulaLayer
	{
		std::array<sf::Glsl::Vec3, 4> ramp{};
		float scale = 1.f;
		float intensity = 0.8f;
		float parallax = 0.12f;
		float seed = 1.f;
	};

	// Where the host star sits, or none at all (a rogue planet far from any star is lit only by the sky).
	struct DeepSkyHostStar
	{
		std::optional<sf::Vector2f> at;   // screen fraction, authored for the unmirrored composition
		bool hide = false;
	};

	// The soft lit cloud layers that fill the screen (skypaint.frag): the ship flies inside a nebula or a dust cloud.
	struct DeepSkyPaint
	{
		bool enabled = false;
		float coverage = 0.5f;
		float intensity = 1.f;
		float centerCalm = 0.6f;
		std::optional<sf::Glsl::Vec3> shadow, mid, lit, accent;
	};

	struct DeepSkyStop
	{
		std::vector<DeepSkyObject> objects;
		DeepSkyPlanet planet;
		DeepSkyHostStar hostStar;
		DeepSkyPaint skyPaint;
		std::vector<DeepSkyNebulaLayer> nebula;
		// The stop's own look: a brighter background ceiling and a stronger grade (0 keeps pipeline.json).
		float capScale = 1.f;
		float gradeContrast = 0.f;
		float gradeSaturation = 0.f;
		// Scales the resolved star count (deep space far from the plane is sparse, a dense cluster field is rich).
		std::optional<float> starScale;
		// Scales the near-field dust motes (inside a dust cloud there are many).
		std::optional<float> motes;
		// The stop lies inside an asteroid belt: rocks stream past at many depths, some close to the ship.
		std::optional<AsteroidBeltSettings> asteroids;
		DeepSkyBand band;
		// Sky gradient tint (display colour) and how far the gradient moves to it, 0..1.
		std::optional<sf::Glsl::Vec3> skyTint;
		float skyTintStrength = 0.f;
		// False pins the objects to the screen (for example to line up with the Sgr A* lens, which never mirrors).
		bool followMirror = true;
		// The stop is a low orbit around one of its planets: the planet becomes a horizon arc (or, with framing "disk", a disk centred on screen) and the sky turns past it. Such a
		// stop draws no objects, nebula layers, sky paint or belt.
		std::optional<OrbitSettings> orbit;
	};

	// deepsky.json: { "presets": { name: stop }, "stops": { stopId: stop | { "use": preset, ...overrides } } }.
	class DeepSkyCatalog
	{
	public:
		bool Load(const std::filesystem::path& file, std::string& error);
		// Same transactional parser as Load, without file I/O. Failure retains the current catalog/version.
		bool Apply(const nlohmann::json& root, std::string& error);
		// Editor transaction: validate changed stops with the same parser; untouched stops keep their compiled data.
		bool ApplyPatch(const nlohmann::json& fragment, const std::vector<std::string>& removed, std::string& error);
		void Replace(const DeepSkyCatalog& catalog) { mStops = catalog.mStops; mPresets = catalog.mPresets; ++mVersion; }
		// nullptr when the stop has no entry (the stop keeps its plain sky).
		const DeepSkyStop* Find(const std::string& stopId) const;
		const DeepSkyStop* FindPreset(const std::string& name) const;
		// Bumped on every successful load, so caches know to re-render.
		unsigned Version() const { return mVersion; }

	private:
		std::map<std::string, DeepSkyStop> mStops;
		std::map<std::string, DeepSkyStop> mPresets;
		unsigned mVersion = 0;
	};

	// Renders a stop's objects once per location (deepsky.frag) in the Milky Way cache's frame. The light goes into the
	// band (GalaxyLayer::Params::extraLight) so the band's dust crosses it; dark objects are multiplied over the background
	// with the band's parallax.
	class DeepSkyLayer
	{
	public:
		struct Params
		{
			const DeepSkyStop* stop = nullptr;
			std::string stopId;
			unsigned catalogVersion = 0;
			float maxBrightness = 0.6f;
			// Objects are authored with the backdrop planet on the right; a mirrored composition mirrors them too.
			bool mirrored = false;
		};

		// Re-renders when the stop, catalog, cap or output size changed. Call before the galaxy draw.
		void Prepare(ShaderLibrary& shaders, sf::Vector2u outputSize, const Params& params);
		// Light of the stop's objects, or nullptr when it has none. Revision changes whenever it is re-rendered.
		const sf::Texture* Light() const { return mValid && mHasLight ? &mLight.getTexture() : nullptr; }
		unsigned Revision() const { return mRevision; }
		// Multiplies the occluders (dark clouds, belts) over the target and adds their own light; parallax should be the band's.
		void DrawShade(sf::RenderTarget& target, sf::Vector2f scroll, float parallax) const;
		void Invalidate() { mValid = false; }

	private:
		bool Render(sf::Shader& shader, const Params& params, sf::Vector2u outputSize);

		sf::RenderTexture mLight;
		sf::RenderTexture mShade;
		sf::RenderTexture mFront;
		bool mHasShade = false;
		bool mHasFront = false;
		bool mHasLight = false;
		unsigned mRevision = 0;
		bool mValid = false;
		sf::Vector2u mOutputSize;
		std::string mStopId;
		bool mMirrored = false;
		unsigned mVersion = 0;
		float mCap = 0.f;
		const sf::Shader* mShader = nullptr;
	};
}
