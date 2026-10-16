#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <SFML/Graphics.hpp>

#include "sky/ShaderLibrary.h"

namespace vega::sky
{
	// Rotating neutron star. Its radio/optical emission comes from two cones along the magnetic axis; as the star spins
	// the cones sweep the sky (lighthouse effect). Drawing the cones as visible beams is a stylization; the pulse period
	// and the synchrotron wind nebula around it are real (Crab-like: bluish-white continuum, filamentary H-alpha shell).
	struct PulsarParams
	{
		sf::Vector2f center;          // output pixels
		float periodSeconds = 1.337f; // PSR B1919+21, the first pulsar found
		float beamLength = 260.f;     // output pixels
		float beamHalfAngleRad = 0.08f;
		float magneticTiltRad = 0.6f; // angle between spin and magnetic axes; the beams precess around the spin axis
		float nebulaRadius = 90.f;
		float timeSeconds = 0.f;
		float maxBrightness = 0.35f;
	};

	// Additive draw into any background-resolution target. Positions are output pixels.
	void DrawPulsar(ShaderLibrary& shaders, sf::RenderTarget& target, const PulsarParams& params);

	// A moon that strayed inside its planet's Roche limit (d < 2.44 R_p (rho_p/rho_m)^(1/3)) is torn apart; the debris
	// keeps orbiting with Keplerian speed (omega ~ r^-1.5), so inner pieces run ahead and the arc shears into a ring.
	class ShatteredMoon
	{
	public:
		ShatteredMoon(unsigned seed, int chunkCount = 70);

		struct Params
		{
			sf::Vector2f planetCenter;   // output pixels
			float orbitRadius = 200.f;   // output pixels, the debris arc's mean radius
			float tilt = 0.35f;          // ellipse squash: ring seen at an angle (y scale)
			float ageSeconds = 0.f;      // time since breakup (drives the shear)
			sf::Vector2f lightDirection{-0.7f, -0.45f}; // toward the host star, screen space
			sf::Color rockColor{120, 110, 100};
			float maxBrightness = 0.35f;
		};

		void Draw(sf::RenderTarget& target, const Params& params) const;

	private:
		struct Chunk
		{
			float radialFraction = 0.f;  // offset from the orbit radius, fraction of it
			float angle0 = 0.f;          // radians at breakup, inside a 60-degree arc
			float size = 3.f;            // output pixels, outline radius
			float spin = 0.f;            // rad/s, the piece's own tumbling
			float shade = 1.f;           // albedo variation
			int vertexCount = 6;         // outline vertices, 5 to 8
			std::array<sf::Vector2f, 8> outline; // unit-size outline offsets
		};

		std::vector<Chunk> mChunks;
		// Scratch buffers reused every frame so Draw does not allocate after the first call.
		mutable std::vector<sf::Vertex> mVertices;
		mutable std::vector<sf::Vertex> mDust;
	};
}
