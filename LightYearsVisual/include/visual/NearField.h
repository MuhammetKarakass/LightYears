#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/ColorRamp.h"

namespace ly::visual
{
	// Sparse near-field debris in front of the far backdrop, behind gameplay: pixel-art rocks with a dark outline,
	// hue-shifted ramp shading lit from the host star, plus drifting dust motes. Strong parallax sells depth.
	class NearField
	{
	public:
		// rockCount and moteCount are the counts at density 1; twice as many are built so density 2 shows more.
		NearField(unsigned seed, int rockCount = 5, int moteCount = 70);
		struct Params
		{
			sf::Vector2f scroll;            // camera motion, output pixels
			float parallax = 0.35f;         // rocks; motes use parallax * 1.6
			float timeSeconds = 0.f;        // slow drift and tumble
			float pixelSize = 3.f;          // output pixels per art pixel (match the world pixel scale)
			sf::Vector2f lightDirection{-0.7f, -0.45f}; // toward the host star, screen space
			const ColorRamp* rockRamp = nullptr;  // stops 0 to 2 shade the rocks; nullptr -> skip rocks
			const ColorRamp* moteRamp = nullptr;
			float maxBrightness = 0.42f;
			float density = 1.f;            // 0..2, scales how many rocks/motes are shown (dust-rich regions -> more)
		};
		void Draw(sf::RenderTarget& target, const Params& params) const;

	private:
		struct Rock
		{
			sf::Vector2f home;                    // fraction of the target size
			sf::Vector2f drift;                   // output pixels per second
			float depth = 1.f;                    // parallax multiplier
			float radius = 6.f;                   // art pixels, outline radius
			float phase = 0.f;
			float spin = 0.f;                     // rad/s, the rock's own tumbling
			float shade = 1.f;                    // albedo variation
			int vertexCount = 6;                  // outline vertices, 6 to 10
			std::array<sf::Vector2f, 10> outline; // unit-size outline offsets
		};

		struct Mote
		{
			sf::Vector2f home;                    // fraction of the target size
			float depth = 1.f;
			float phase = 0.f;
			float speed = 0.3f;                   // rad/s of the sine drift
			bool bright = false;                  // ramp stop 3 instead of 2
			float alpha = 0.5f;
		};

		// Per-Draw values the shapes need, so the helpers take one argument for them.
		struct Frame
		{
			float pixel = 1.f;
			float time = 0.f;
			float cap = 0.42f;
			sf::Vector2f light{0.f, -1.f};
			const ColorRamp* rockRamp = nullptr;
			const ColorRamp* moteRamp = nullptr;
		};

		void AddRock(const Rock& rock, sf::Vector2f center, const Frame& frame) const;
		void AddMote(const Mote& mote, sf::Vector2f origin, const Frame& frame) const;

		std::vector<Rock> mRocks;
		std::vector<Mote> mMotes;
		// Scratch buffer reused every frame so Draw does not allocate after the first call.
		mutable std::vector<sf::Vertex> mVertices;
	};
}
