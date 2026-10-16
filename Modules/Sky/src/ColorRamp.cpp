#include "sky/ColorRamp.h"

#include <algorithm>
#include <cmath>

namespace vega::sky
{
	ColorRamp::ColorRamp(std::vector<ColorStop> stops)
		: mStops(std::move(stops))
	{
		std::stable_sort(mStops.begin(), mStops.end(), [](const ColorStop& a, const ColorStop& b) { return a.t < b.t; });
	}

	sf::Color ColorRamp::Sample(float t) const
	{
		if (mStops.empty())
		{
			return sf::Color::Magenta;
		}
		if (t <= mStops.front().t)
		{
			return mStops.front().color;
		}
		for (size_t i = 1; i < mStops.size(); ++i)
		{
			if (t <= mStops[i].t)
			{
				const ColorStop& prev = mStops[i - 1];
				float span = mStops[i].t - prev.t;
				return LerpColor(prev.color, mStops[i].color, span > 0.f ? (t - prev.t) / span : 1.f);
			}
		}
		return mStops.back().color;
	}

	sf::Color LerpColor(sf::Color a, sf::Color b, float k)
	{
		k = std::clamp(k, 0.f, 1.f);
		auto mix = [k](std::uint8_t x, std::uint8_t y) { return static_cast<std::uint8_t>(std::lround(x + (y - x) * k)); };
		return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a));
	}

	sf::Color LimitBrightness(sf::Color color, float cap)
	{
		float peak = std::max({color.r, color.g, color.b}) / 255.f;
		if (peak <= cap || peak <= 0.f)
		{
			return color;
		}
		float k = cap / peak;
		return sf::Color(static_cast<std::uint8_t>(color.r * k), static_cast<std::uint8_t>(color.g * k), static_cast<std::uint8_t>(color.b * k), color.a);
	}
}
