#pragma once

#include <SFML/Graphics/Color.hpp>

#include <algorithm>
#include <cstdint>

namespace vega
{
	inline float Saturate(float value) { return std::clamp(value, 0.f, 1.f); }

	inline sf::Color WithAlpha(const sf::Color& color, float multiplier)
	{
		sf::Color result = color;
		result.a = static_cast<std::uint8_t>(std::clamp(static_cast<float>(color.a) * Saturate(multiplier), 0.f, 255.f));
		return result;
	}
}
