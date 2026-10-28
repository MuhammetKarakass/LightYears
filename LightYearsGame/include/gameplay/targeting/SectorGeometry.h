#pragma once

#include <SFML/System/Vector2.hpp>

namespace vega::targeting
{
	// Inclusive circle/sector overlap, including the radial edges and end arc.
	bool CircleOverlapsSector(const sf::Vector2f& center, float circleRadius, const sf::Vector2f& origin, const sf::Vector2f& direction, float radius, float halfAngleRadians);
}
