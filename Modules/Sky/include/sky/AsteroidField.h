#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

#include "sky/ShaderLibrary.h"

namespace vega::sky
{
	// A stop's asteroid belt seen from inside it (deepsky.json "asteroids"): rocks at many depths stream along the belt,
	// tumble, and slide against the camera with depth-scaled parallax, so the near ones pass close to the ship.
	struct AsteroidBeltSettings
	{
		int count = 60;
		float angleDeg = -12.f;       // belt direction on screen
		float center = 0.55f;         // belt centre line, fraction of the screen height
		float width = 0.35f;          // half thickness, fraction of the screen height
		float speed = 1.f;            // orbital drift along the belt
		float nearest = 0.8f;         // depth of the closest rocks (0..1): larger = bigger and faster past the camera
		sf::Glsl::Vec3 dark{0.06f, 0.05f, 0.06f};
		sf::Glsl::Vec3 mid{0.42f, 0.36f, 0.32f};
		sf::Glsl::Vec3 light{0.95f, 0.85f, 0.72f};
		sf::Glsl::Vec3 haze{0.08f, 0.08f, 0.14f}; // far rocks fade toward this sky colour

		bool operator==(const AsteroidBeltSettings& other) const;
		bool operator!=(const AsteroidBeltSettings& other) const { return !(*this == other); }
	};

	class AsteroidField
	{
	public:
		void Build(unsigned seed, const AsteroidBeltSettings& settings);
		bool Empty() const { return mRocks.empty(); }

		struct Params
		{
			sf::Vector2f scroll;          // camera motion, output pixels
			float timeSeconds = 0.f;
			sf::Vector2f lightDirection{-0.7f, -0.45f}; // toward the host star, screen space
			float maxBrightness = 0.6f;
			bool mirrored = false;
		};
		// One draw call: every rock is a rotated quad whose vertex colour carries its seed, pixel radius, local light angle and
		// depth for asteroid.frag. Far rocks first.
		void Draw(ShaderLibrary& shaders, sf::RenderTarget& target, const Params& params) const;

	private:
		struct Rock
		{
			float along = 0.f;   // 0..1 position along the belt
			float across = 0.f;  // -1..1 offset across the belt
			float depth = 0.5f;  // 0 far .. 1 near
			float size = 0.02f;  // radius, fraction of the screen height
			float spin = 0.f;    // rad/s
			float phase = 0.f;
			float seed = 0.f;    // 0..1
		};

		AsteroidBeltSettings mSettings;
		std::vector<Rock> mRocks;
		mutable std::vector<sf::Vertex> mVertices;
	};
}
