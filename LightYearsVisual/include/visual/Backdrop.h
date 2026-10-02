#pragma once

#include <vector>

#include <SFML/Graphics.hpp>

#include "visual/ColorRamp.h"
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
	void DrawNebula(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const NebulaParams& params);

	struct PlanetParams
	{
		sf::Vector2f center;
		float radius = 100.f;
		// output pixels per world-layer pixel (outline and rim width)
		float pixelScale = 1.f;
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
	};

	// Procedural shaded planet quantized to a 5-tone ramp. Draw it into the World layer (pixel look).
	void DrawPlanet(ShaderLibrary& shaders, sf::RenderTarget& target, const ColorRamp& ramp, const PlanetParams& params);

	// Sparse sharp stars with three parallax depths, tinted by a ramp.
	class StarField
	{
	public:
		StarField(unsigned seed, int count);
		void Draw(sf::RenderTarget& target, sf::Vector2f scroll, float timeSeconds, const ColorRamp& tint, float maxBrightness) const;

	private:
		struct Star
		{
			sf::Vector2f unit;
			float depth = 0.f;
			float tint = 0.f;
			float phase = 0.f;
		};

		std::vector<Star> mStars;
	};
}
