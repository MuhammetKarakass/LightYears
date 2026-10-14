#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "visual/Backdrop.h"
#include "visual/Galaxy.h"
#include "visual/ShaderLibrary.h"

namespace ly::visual
{
	// Framing of the orbited planet: a horizon arc at one screen edge, or the whole disk centred while the camera looks down at it.
	enum class OrbitFraming
	{
		Horizon,
		Disk
	};

	// A ring around the planet's equator, drawn by DrawPlanetRing; its plane turns as the orbit does.
	struct OrbitRing
	{
		float inner = 1.35f;                   // inner edge, planet radii
		float outer = 2.3f;                    // outer edge, planet radii
		float tiltDeg = 30.f;                  // angle between the ring plane and the orbit plane
		float nodeDeg = 0.f;                   // orbit longitude of the ring's node
		float opacity = 0.85f;                 // ring opacity

		bool operator==(const OrbitRing& other) const;
		bool operator!=(const OrbitRing& other) const { return !(*this == other); }
	};

	// Geometry and motion of a low-orbit stop (deepsky.json "orbit"); angles in degrees as authored.
	struct OrbitSettings
	{
		int planet = 0;                        // which of the route stop's planets we orbit
		std::optional<PlanetKind> kind;        // art override of the classified kind (for a planet without a known radius)
		float horizonAngleDeg = -90.f;         // screen direction from the planet centre to the horizon apex (y down; -90 = planet below);
		                                       // in disk framing it only sets the orbit's direction on screen, the sky flowing away from it
		float coverage = 0.3f;                 // how far the planet reaches into the screen, fraction of the output height
		float radius = 2.5f;                   // horizon framing: planet radius in output heights (curvature of the horizon)
		float atmosphere = 0.02f;              // atmosphere height, fraction of the output height
		float detail = 2.f;                    // surface noise frequency (PlanetParams::surfaceScale)
		float brightness = 1.f;                // multiplies the pipeline's planet brightness
		float spin = 0.f;                      // the planet's own rotation over the orbit rate (+ same way as the orbit: the ground runs slower than the sky; - faster; 0 = tidally locked)
		float fieldOfViewDeg = 70.f;           // sky angle spanned by the output height
		float degreesPerMinute = 8.f;          // orbit rate: how fast the sky turns
		float startDeg = 0.f;                  // orbit longitude in view at the location start
		float starDeg = 20.f;                  // host star's orbit longitude
		float starAcrossDeg = 0.f;             // host star's angle across the orbit
		float bandTiltDeg = 30.f;              // angle between the galactic plane and the orbit plane (0 = the band runs along the flow)
		float bandNodeDeg = 0.f;               // orbit longitude where the band crosses the orbit plane
		float centerDeg = 0.f;                 // galactic centre's angle along the band from that node
		OrbitFraming framing = OrbitFraming::Horizon; // horizon arc at a screen edge, or the planet centred on screen
		float diskRadius = 0.22f;              // disk framing: planet radius, fraction of the output height
		std::optional<OrbitRing> ring;         // rings around the planet's equator
		bool moon = false;                     // a small moon circles the planet (DrawPlanetMoon)

		bool operator==(const OrbitSettings& other) const;
		bool operator!=(const OrbitSettings& other) const { return !(*this == other); }
	};

	// A low orbit: the planet is a horizon arc at one screen edge (or a disk centred on screen) and the sky turns past as one rigid panorama, since everything
	// at infinity moves with the same angular speed. The Milky Way is baked once per location into a vertically periodic texture
	// covering a full turn.
	class OrbitSky
	{
	public:
		// The orbit's screen frame at one moment.
		struct View
		{
			sf::Vector2f output;            // output size, px
			sf::Vector2f flow{0.f, 1.f};    // unit on-screen direction the sky moves (toward the planet)
			float pixelsPerRadian = 1.f;
			float phaseRad = 0.f;           // orbit longitude at the screen centre
			sf::Vector2f drift;             // camera parallax shift of the sky, px

			sf::Vector2f Across() const { return {-flow.y, flow.x}; }
			// Screen point of a sky direction: longitude along the orbit, latitude across it.
			sf::Vector2f ScreenPoint(float longitudeRad, float acrossRad) const;
			// Light vector of a sky direction in scene axes (x right, y down, z toward the viewer): a star straight ahead lights
			// the planet from behind, one past the horizon leaves its visible cap in night, one behind the viewer lights it fully.
			sf::Vector3f Direction(float longitudeRad, float acrossRad) const;
			// Offset for StarField::DrawRigid.
			sf::Vector2f StarOffset() const;
		};

		// The frame at secondsSinceStart; skyDrift is the camera parallax of the sky, px; motion picks how the sky and the surface turn.
		static View MakeView(const OrbitSettings& orbit, sf::Vector2f output, float secondsSinceStart, sf::Vector2f skyDrift);
		// Places the orbited planet for the framing (ray-cast disk, or orthographic horizon sphere), with its orbit rotation and ring; the caller sets its other fields.
		static void PlacePlanet(PlanetParams& planet, const OrbitSettings& orbit, const View& view, sf::Vector2f planetDrift);
		// Light vector of the host star for the orbited planet: the disk's sky direction, or the horizon's frame lit from its limb.
		static sf::Vector3f PlanetLight(const OrbitSettings& orbit, const View& view, float longitudeRad, float acrossRad);
		// Apparent radius of the host star in output pixels: its angular radius times the pixels per radian.
		static float StarRadius(const View& view, float starRadiusSolar, float distanceAU);
		// Bakes the panorama when the band, orbit or output changed, then adds the visible window onto target.
		void DrawSky(ShaderLibrary& shaders, sf::RenderTarget& target, const GalaxyLayer::Params& band, const OrbitSettings& orbit, const View& view);
		void Invalidate() { mValid = false; }

	private:
		bool Bake(sf::Shader& shader, const GalaxyLayer::Params& band, const OrbitSettings& orbit, sf::Vector2u outputSize);

		sf::RenderTexture mPanorama;
		bool mValid = false;
		sf::Vector2u mOutputSize;
		unsigned mSeed = 0;
		float mCap = 0.f;
		MilkyWaySettings mSettings;
		BandStyle mStyle;
		// The orbit fields that change the bake.
		float mFieldOfView = 0.f;
		float mTilt = 0.f;
		float mNode = 0.f;
		float mCenter = 0.f;
		const sf::Shader* mShader = nullptr;
	};
}
