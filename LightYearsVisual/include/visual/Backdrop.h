#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/Astro.h"
#include "visual/ColorRamp.h"
#include "visual/GalaxyModel.h"
#include "visual/PipelineSettings.h"
#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	struct NebulaParams
	{
		sf::Vector2f scroll;
		// noise cycles per pixel; small values give large soft clouds
		float scale = 0.0035f;
		float intensity = 0.6f;
		float seed = 0.f;
		float maxBrightness = 0.35f;
	};

	// Vertical top -> bottom gradient over the whole target, brightness-capped to the background tier.
	void DrawGradient(sf::RenderTarget& target, sf::Color top, sf::Color bottom, float maxBrightness);
	// Soft domain-warped cloud layer colored by the ramp (4 samples, shadow -> light).
	// The quad covers the target view, so it also works with scaled intermediate targets.
	void DrawNebula(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const NebulaParams& params);

	// Planet classes, from the first planet's radius and equilibrium temperature (ClassifyPlanet).
	enum class PlanetKind
	{
		Terran,
		Ocean,
		Desert,
		Lava,
		Ice,
		GasGiant,
		HotJupiter,
		IceGiant
	};

	// A ring around the equator: DrawPlanet casts its shadow on the surface and DrawPlanetRing draws it.
	struct PlanetRing
	{
		// unit normal in scene axes
		sf::Vector3f normal{0.f, -0.94f, 0.34f};
		// radii in planet radii
		float inner = 1.35f;
		float outer = 2.3f;
		float opacity = 0.85f;
	};

	// An orbit camera that ray-casts a planet: each pixel's ray follows the sky's mapping (OrbitSky::View), so the planet and the sky share one lens.
	struct PlanetRayCamera
	{
		// unit direction from the camera to the planet centre, scene axes
		sf::Vector3f centerDir{0.f, 0.f, -1.f};
		// camera distance to the planet centre, planet radii (> 1)
		float distance = 1.f;
		// screen point of the view centre, px
		sf::Vector2f viewCenter;
		// unit on-screen direction the sky flows
		sf::Vector2f flow{0.f, 1.f};
		// the sky's pixels per radian
		float pixelsPerRadian = 1.f;
	};

	struct PlanetParams
	{
		sf::Vector2f center;
		float radius = 100.f;
		// output pixels per world-layer pixel (outline and rim width)
		float pixelScale = 1.f;
		// Output pixels per surface sample; affects texture style without snapping the center or silhouette.
		float surfacePixelScale = 1.f;
		// surface -> light in scene axes (x right, y down, z toward the viewer)
		sf::Vector3f light{0.75f, -0.45f, 0.5f};
		float rotation = 0.f;
		float seed = 3.f;
		// 0 = hard tone bands only, 1 = wide dither
		float dither = 0.3f;
		// scales the final color; lower it so a backdrop planet stays below gameplay in brightness
		float brightness = 1.f;
		// 1 = ramp colors as authored, 0 = grayscale
		float saturation = 1.f;
		// Blends a distant planet into the background haze; 1 keeps it opaque.
		float opacity = 1.f;
		// Colour of the host star (display space) that tints the lit side.
		sf::Glsl::Vec3 lightColor{1.f, 1.f, 1.f};
		// Strength of the lit side, derived from the host star's irradiance; the night side is not affected.
		float lightIntensity = 1.f;
		// Surface kind: bands, storm and lava cracks follow it. Set with SetPlanetKind, which also sets the two below.
		PlanetKind kind = PlanetKind::Terran;
		// Seconds; drives the cloud drift and the storm swirl.
		float timeSeconds = 0.f;
		// Cloud cover 0..1 (alpha up to 0.6); 0 = no clouds.
		float cloudAmount = 0.5f;
		// Limb glow colour (display space); black = no glow.
		sf::Glsl::Vec3 atmosphereColor{0.4f, 0.6f, 1.f};
		// Unit spin axis in scene axes; the default turns the surface as it always has.
		sf::Vector3f spinAxis{0.f, 1.f, 0.f};
		// Atmosphere halo falloff in planet radii; the halo reaches 0.14/0.03 of it past the limb.
		float haloWidth = 0.03f;
		// Frequency of the surface and cloud noise; > 1 gives a big close planet more detail.
		float surfaceScale = 1.f;
		// Rings around the equator; DrawPlanet casts their shadow, DrawPlanetRing draws them.
		std::optional<PlanetRing> ring;
		// Set when the planet is ray-cast from an orbit camera; DrawPlanet then shades the sphere per pixel.
		std::optional<PlanetRayCamera> rayCamera;
		// Exact rotation of the surface about orbitAxis by orbitAngle (scene axes, radians), from the camera frame to the planet's frame.
		sf::Vector3f orbitAxis{0.f, 1.f, 0.f};
		float orbitAngle = 0.f;
	};

	// Kind from the first planet's radius (Earth radii) and equilibrium temperature (K). A value <= 0 is unknown and
	// picks by seed; the rule for each class is in Backdrop.cpp.
	PlanetKind ClassifyPlanet(float radiusEarth, float eqTempK, unsigned seed);
	// Kind from its name (terran, ocean, desert, lava, ice, gasGiant, hotJupiter, iceGiant); nullopt for any other name.
	std::optional<PlanetKind> ParsePlanetKind(const std::string& name);
	// The 5-stop ramp (shadow -> light, hue-shifted shadows) of a kind, for DrawPlanet.
	ColorRamp PlanetRamp(PlanetKind kind);
	// Sets the kind and the look that follows from it: limb glow colour and cloud cover (none for Lava).
	void SetPlanetKind(PlanetParams& params, PlanetKind kind);

	// Procedural shaded planet quantized to a 5-tone ramp. Background gives a smooth limb; World gives a pixel look.
	void DrawPlanet(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const PlanetParams& params);
	// Planet ring (planetring.frag). Call with front = false before DrawPlanet and front = true after it; the half nearer the
	// viewer (z >= 0) is the front. Does nothing without planet.ring. With a ray camera the ring is one pass: front = false does nothing.
	void DrawPlanetRing(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const PlanetParams& planet, bool front);
	// Small moon (0.18 planet radii) on a squashed orbit (1.7 radii, 40 s period) drawn with the planet shader. Call with
	// front = false before DrawPlanet and front = true after it: the moon is behind the planet while sin(phase) < 0.
	// The seed picks its phase and its kind (Ice or Desert); it has no clouds and no glow. Lit like the planet.
	void DrawPlanetMoon(ShaderLibrary& shaders, sf::RenderTarget& target, const PlanetParams& planet, unsigned seed, float timeSeconds, bool front);

	// The star of the current system as seen from the play area.
	struct HostStar
	{
		float temperatureK = 5772.f;   // the Sun
		float luminositySolar = 1.f;
		float radiusSolar = 1.f;
		float distanceAU = 1.f;        // observer distance from the star
	};
	// Irradiance relative to Earth's: L / d^2.
	float HostStarIrradiance(const HostStar& star);
	// Apparent radius in output pixels: radiusAt1AU * R / d (the Sun at 1 AU spans ~0.27 deg).
	float HostStarApparentRadius(const HostStar& star, float radiusAt1AUPixels);

	struct HostStarParams
	{
		sf::Vector2f center;           // output pixels
		float radius = 10.f;           // disc radius, output pixels
		float glowRadius = 120.f;      // halo extent, output pixels
		sf::Glsl::Vec3 color;          // display-space colour (blackbody)
		float intensity = 1.f;         // 0..1 of maxBrightness
		float maxBrightness = 0.35f;
	};

	// Limb-darkened disc (I = 1 - u(1 - mu), u = 0.6) with a soft scattered-light halo, additive.
	void DrawHostStar(ShaderLibrary& shaders, sf::RenderTarget& target, const HostStarParams& params);

	// Sky colours of one region, display space 0..1 per channel: shadow (darkest), mid, lit (rims facing the star), accent.
	struct SkyPalette
	{
		sf::Glsl::Vec3 shadow;
		sf::Glsl::Vec3 mid;
		sf::Glsl::Vec3 lit;
		sf::Glsl::Vec3 accent;
	};

	// Milky Way band seen from inside the disk, drawn into the (transparent) nebula target before the nebula clouds.
	// scroll = camera motion in output pixels; settings.parallax is applied inside. A non-null tint (the region palette)
	// takes the band's warm and blue colours toward its lit and mid colours.
	void DrawMilkyWay(ShaderLibrary& shaders, sf::RenderTarget& target, const galaxy::SkyState& sky, const MilkyWaySettings& settings, sf::Vector2f scroll, float maxBrightness, const SkyPalette* tint = nullptr);

	// A comet whose two tails point away from the host star: a straight blue ion tail and a curved dust tail.
	struct CometParams
	{
		sf::Vector2f head;             // nucleus, output pixels
		sf::Vector2f starAt;           // host star, output pixels: the tails point away from it
		float length = 300.f;          // tail length, output pixels
		float width = 14.f;            // coma radius, output pixels
		float bend = 1.f;              // dust tail curvature and side (-1..1)
		float timeSeconds = 0.f;
		float maxBrightness = 0.6f;
	};
	void DrawComet(ShaderLibrary& shaders, sf::RenderTarget& target, const CometParams& params);

	// Region colour script from the galaxy position: blend of the region palettes above by bulgeFraction, armProximity and
	// nearestArm (interarm when armProximity is low).
	SkyPalette SkyPaletteFor(const galaxy::SkyState& sky);

	struct SkyPaintParams
	{
		sf::Vector2f scroll;          // camera motion, output pixels
		sf::Vector2f lightPos;        // host star, output pixels (may be off-screen)
		SkyPalette palette;
		float coverage = 0.55f;       // 0..1 how much of the sky the clouds fill
		float intensity = 1.f;
		float centerCalm = 0.45f;     // 0..1 dims cloud detail around the screen centre where the player fights
		float timeSeconds = 0.f;      // very slow drift
		float seed = 0.f;
		float maxBrightness = 0.42f;
		// Parallax of the three layers, output px of drift per output px of camera motion (from SkyPaintSettings).
		float farParallax = 0.015f;
		float midParallax = 0.035f;
		float nearParallax = 0.07f;
	};

	// Three parallax layers of lit soft clouds (skypaint.frag), drawn premultiplied into the nebula target before the Milky Way.
	void DrawSkyPaint(ShaderLibrary& shaders, sf::RenderTarget& nebulaTarget, const SkyPaintParams& params);

	// Background stars from the galaxy model: colour from blackbody temperature, brightness from apparent magnitude
	// (logarithmic, so faint stars dominate), size from brightness, parallax independent of brightness. Stars do not
	// twinkle (scintillation is atmospheric); a few are variable (Cepheid-like pulsators, eclipsing binaries) with
	// periods compressed for play. Band stars follow the Milky Way plane and are dimmed/reddened by dust near its centre.
	class StarField
	{
	public:
		explicit StarField(unsigned seed);
		// Regenerates the stars for a sky; deterministic for the same seed and inputs. Call when the location or the
		// settings change, not every frame.
		void Build(const galaxy::SkyState& sky, const StarFieldSettings& stars, const MilkyWaySettings& band);
		// Adds a star cluster after Build, drawn exactly like the field stars: centre in 0..1 of the target, radius as a
		// fraction of the target height, aspect = width / height of the target. Globular (open = false): a dense Plummer
		// crowd of old yellow-white stars whose brightest are orange giants. Open: a loose group of young blue-white stars.
		void AddCluster(sf::Vector2f centre, float radius, bool open, float aspect, unsigned seed);
		// scroll = camera motion in output pixels; each star applies its own parallax and wraps over the target.
		void Draw(sf::RenderTarget& target, sf::Vector2f scroll, float timeSeconds, float maxBrightness) const;
		// Every field star moves by the same offset (a sky turning as one piece, as seen from a low orbit) and wraps over
		// the target. Band stars are skipped (build with the band disabled to get none).
		void DrawRigid(sf::RenderTarget& target, sf::Vector2f offset, float timeSeconds, float maxBrightness) const;
		// Ship velocity as a fraction of light speed and its on-screen direction (unit vector, y down). 0 = at rest (the
		// default, identical to the current look). fieldOfViewRad is how much of the sky the screen spans across the
		// direction of motion.
		void SetMotion(float beta, sf::Vector2f direction, float fieldOfViewRad = 1.2f);
		std::size_t Count() const { return mStars.size(); }

	private:
		enum class StarSize : std::uint8_t
		{
			Point,
			Core,
			Spiked
		};

		enum class Variability : std::uint8_t
		{
			None,
			Cepheid,
			Eclipsing
		};

		struct Star
		{
			// field stars: x, y in 0..1 of the target; band stars: x = position along the band in 0..1
			sf::Vector2f unit;
			// band stars: offset from the band centre line, in target heights
			float across = 0.f;
			bool band = false;
			// the brightest ~2% get a soft halo in Draw
			bool halo = false;
			float parallax = 0.f;
			// apparent magnitude after dust extinction, before variability
			float magnitude = 0.f;
			float level = 0.f;
			float phase = 0.f;
			float period = 1.f;
			// effective temperature after dust reddening; Doppler shift scales it while the ship moves
			float temperatureK = 5000.f;
			// display colour (sRGB), converted once in Build
			sf::Color color;
			StarSize size = StarSize::Point;
			Variability variability = Variability::None;
		};

		// Magnitude and level of a star at timeSeconds; only variable stars change them.
		void Vary(const Star& star, float timeSeconds, float& magnitude, float& level) const;
		// Adds one star at screen point p (before aberration) with its rects, halo, streak and spikes. Shared by Draw and DrawRigid.
		void Emit(const Star& star, sf::Vector2f p, float magnitude, float level, sf::Vector2f size, float maxBrightness) const;

		unsigned mSeed;
		StarFieldSettings mStarSettings;
		MilkyWaySettings mBand;
		std::vector<Star> mStars;
		mutable std::vector<sf::Vertex> mVertices;
		// ship motion (see SetMotion): beta = v/c, unit on-screen direction of motion, screen span across it
		float mBeta = 0.f;
		sf::Vector2f mDirection{1.f, 0.f};
		float mFov = 1.2f;
	};
}
