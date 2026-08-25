#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>

namespace ly::targeting::swept
{
	inline sf::FloatRect SegmentBounds(
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd,
		float expansion = 0.f
	)
	{
		const float safeExpansion = std::max(0.f, expansion);
		const float left = std::min(segmentStart.x, segmentEnd.x) - safeExpansion;
		const float top = std::min(segmentStart.y, segmentEnd.y) - safeExpansion;
		const float right = std::max(segmentStart.x, segmentEnd.x) + safeExpansion;
		const float bottom = std::max(segmentStart.y, segmentEnd.y) + safeExpansion;
		return { { left, top }, { right - left, bottom - top } };
	}

	inline sf::FloatRect RadiusBounds(
		const sf::Vector2f& center,
		float radius
	)
	{
		const float safeRadius = std::max(0.f, radius);
		return {
			{ center.x - safeRadius, center.y - safeRadius },
			{ safeRadius * 2.f, safeRadius * 2.f }
		};
	}

	inline float DistanceSquaredToSegment(
		const sf::Vector2f& point,
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd
	)
	{
		const sf::Vector2f segment = segmentEnd - segmentStart;
		const float segmentLengthSquared = segment.x * segment.x + segment.y * segment.y;
		if (segmentLengthSquared <= 0.0001f)
		{
			const sf::Vector2f delta = point - segmentStart;
			return delta.x * delta.x + delta.y * delta.y;
		}

		const sf::Vector2f pointOffset = point - segmentStart;
		const float normalizedProjection = std::clamp(
			(pointOffset.x * segment.x + pointOffset.y * segment.y) / segmentLengthSquared,
			0.f,
			1.f
		);
		const sf::Vector2f closestPoint = segmentStart + segment * normalizedProjection;
		const sf::Vector2f delta = point - closestPoint;
		return delta.x * delta.x + delta.y * delta.y;
	}

	inline float SegmentProjectionFraction(
		const sf::Vector2f& point,
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd
	)
	{
		const sf::Vector2f segment = segmentEnd - segmentStart;
		const float segmentLengthSquared = segment.x * segment.x + segment.y * segment.y;
		if (segmentLengthSquared <= 0.0001f)
		{
			return 0.f;
		}
		const sf::Vector2f offset = point - segmentStart;
		return std::clamp(
			(offset.x * segment.x + offset.y * segment.y) / segmentLengthSquared,
			0.f,
			1.f
		);
	}

	// Returns the first point where a segment reaches a rotated box expanded by
	// a moving projectile radius. Keeping this math shared prevents every wall,
	// mirror, or gate from implementing its own slightly different ricochet
	// normal calculation.
	inline bool SegmentIntersectsExpandedOrientedBox(
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd,
		const sf::Vector2f& boxCenter,
		const sf::Vector2f& boxHalfExtents,
		float rotationRadians,
		float expansion,
		float& outFraction,
		sf::Vector2f& outSurfaceNormal
	)
	{
		const float safeExpansion = std::max(0.f, expansion);
		const float cosine = std::cos(rotationRadians);
		const float sine = std::sin(rotationRadians);
		const auto toLocal = [&](const sf::Vector2f& point)
		{
			const sf::Vector2f offset = point - boxCenter;
			return sf::Vector2f{
				cosine * offset.x + sine * offset.y,
				-sine * offset.x + cosine * offset.y
			};
		};
		const auto toWorldDirection = [&](const sf::Vector2f& direction)
		{
			return sf::Vector2f{
				cosine * direction.x - sine * direction.y,
				sine * direction.x + cosine * direction.y
			};
		};

		const sf::Vector2f start = toLocal(segmentStart);
		const sf::Vector2f end = toLocal(segmentEnd);
		const sf::Vector2f delta = end - start;
		const sf::Vector2f extent{
			std::max(0.f, boxHalfExtents.x) + safeExpansion,
			std::max(0.f, boxHalfExtents.y) + safeExpansion
		};
		if (extent.x <= 0.f || extent.y <= 0.f)
		{
			return false;
		}

		float enter = 0.f;
		float exit = 1.f;
		sf::Vector2f localNormal{};
		const auto clipAxis = [&](float origin, float direction, float minimum,
			float maximum, const sf::Vector2f& negativeNormal,
			const sf::Vector2f& positiveNormal)
		{
			if (std::abs(direction) <= 0.0001f)
			{
				return origin >= minimum && origin <= maximum;
			}
			const float inverseDirection = 1.f / direction;
			float nearTime = (minimum - origin) * inverseDirection;
			float farTime = (maximum - origin) * inverseDirection;
			sf::Vector2f nearNormal = negativeNormal;
			if (nearTime > farTime)
			{
				std::swap(nearTime, farTime);
				nearNormal = positiveNormal;
			}
			if (nearTime > enter)
			{
				enter = nearTime;
				localNormal = nearNormal;
			}
			exit = std::min(exit, farTime);
			return enter <= exit;
		};

		if (!clipAxis(start.x, delta.x, -extent.x, extent.x, { -1.f, 0.f }, { 1.f, 0.f }) ||
			!clipAxis(start.y, delta.y, -extent.y, extent.y, { 0.f, -1.f }, { 0.f, 1.f }) ||
			enter < 0.f || enter > 1.f)
		{
			return false;
		}

		outFraction = enter;
		outSurfaceNormal = toWorldDirection(localNormal);
		return true;
	}

	// A segment-vs-expanded-AABB test shared by piercing projectiles and forced
	// movement. Bounce-specific contact normals remain feature-local.
	inline bool SegmentIntersectsExpandedBounds(
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd,
		const sf::FloatRect& bounds,
		float expansion
	)
	{
		const float safeExpansion = std::max(0.f, expansion);
		if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
		{
			return DistanceSquaredToSegment(bounds.position, segmentStart, segmentEnd) <=
				safeExpansion * safeExpansion;
		}

		const float left = bounds.position.x - safeExpansion;
		const float right = bounds.position.x + bounds.size.x + safeExpansion;
		const float top = bounds.position.y - safeExpansion;
		const float bottom = bounds.position.y + bounds.size.y + safeExpansion;
		const sf::Vector2f delta = segmentEnd - segmentStart;
		float enter = 0.f;
		float exit = 1.f;
		const auto clip = [&](float start, float direction, float minimum, float maximum)
		{
			if (std::abs(direction) <= 0.0001f)
			{
				return start >= minimum && start <= maximum;
			}
			float first = (minimum - start) / direction;
			float second = (maximum - start) / direction;
			if (first > second)
			{
				std::swap(first, second);
			}
			enter = std::max(enter, first);
			exit = std::min(exit, second);
			return enter <= exit;
		};

		return clip(segmentStart.x, delta.x, left, right) &&
			clip(segmentStart.y, delta.y, top, bottom);
	}
}
