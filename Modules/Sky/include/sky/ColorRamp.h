#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>

namespace vega::sky
{
	struct ColorStop
	{
		float t = 0.f;
		sf::Color color;
	};

	// Ordered shadow -> mid -> light stops. Shadows are authored hue-shifted, never gray.
	class ColorRamp
	{
	public:
		ColorRamp() = default;
		explicit ColorRamp(std::vector<ColorStop> stops);

		sf::Color Sample(float t) const;
		const std::vector<ColorStop>& Stops() const { return mStops; }
		bool Empty() const { return mStops.empty(); }

	private:
		std::vector<ColorStop> mStops;
	};

	sf::Color LerpColor(sf::Color a, sf::Color b, float k);
	// Scales the color down (hue kept) so its brightest channel is at most cap (0..1).
	sf::Color LimitBrightness(sf::Color color, float cap);
}
