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
