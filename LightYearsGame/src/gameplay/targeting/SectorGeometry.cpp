#include "gameplay/targeting/SectorGeometry.h"
#include <algorithm>
#include <cmath>

namespace vega::targeting
{
	bool CircleOverlapsSector(const sf::Vector2f& center, float circleRadius, const sf::Vector2f& origin, const sf::Vector2f& direction, float radius, float halfAngleRadians)
	{
		const auto delta = center - origin;
		const float length = std::hypot(direction.x, direction.y), distance = std::hypot(delta.x, delta.y);
		if (length <= .0001f || radius < 0.f || circleRadius < 0.f || distance > radius + circleRadius) return false;
		if (distance <= circleRadius) return true;
		const auto forward = direction / length;
		const float angle = std::acos(std::clamp((delta.x * forward.x + delta.y * forward.y) / distance, -1.f, 1.f));
		if (angle <= halfAngleRadians) return true;
		for (const float sign : { -1.f, 1.f })
		{
			const float c = std::cos(halfAngleRadians), s = std::sin(halfAngleRadians) * sign;
			const sf::Vector2f edge{ forward.x * c - forward.y * s, forward.x * s + forward.y * c };
			const float along = std::clamp(delta.x * edge.x + delta.y * edge.y, 0.f, radius);
			const auto nearest = delta - edge * along;
			if (nearest.x * nearest.x + nearest.y * nearest.y <= circleRadius * circleRadius) return true;
		}
		return false;
	}
}
