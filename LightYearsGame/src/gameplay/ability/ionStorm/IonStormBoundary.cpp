#include "gameplay/ability/ionStorm/IonStormBoundary.h"

#include "framework/MathUtility.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;
		constexpr float TwoPi = Pi * 2.f;

		float ClampFinite(float value, float fallback)
		{
			return std::isfinite(value) ? value : fallback;
		}
	}

	IonStormBoundary IonStormBoundary::Generate(
		int controlPointCount,
		float innerCoreRadius,
		float outerMinRadius,
		float outerMaxRadius
	)
	{
		IonStormBoundary boundary;
		boundary.mInnerCoreRadius = std::max(
			0.f,
			ClampFinite(innerCoreRadius, 250.f)
		);
		boundary.mOuterMinRadius = std::max(
			boundary.mInnerCoreRadius,
			ClampFinite(outerMinRadius, boundary.mInnerCoreRadius)
		);
		boundary.mOuterMaxRadius = std::max(
			boundary.mOuterMinRadius,
			ClampFinite(outerMaxRadius, boundary.mOuterMinRadius)
		);

		const std::size_t count = static_cast<std::size_t>(std::max(3, controlPointCount));
		boundary.mControlRadii.resize(count);
		for (float& radius : boundary.mControlRadii)
		{
			radius = RandRange(
				boundary.mOuterMinRadius,
				boundary.mOuterMaxRadius
			);
		}

		// Low-frequency circular smoothing prevents the edge from becoming a
		// star-shaped zigzag while preserving a different silhouette per cast.
		for (int pass = 0; pass < 3; ++pass)
		{
			List<float> smoothed = boundary.mControlRadii;
			for (std::size_t index = 0; index < count; ++index)
			{
				const std::size_t previous = index == 0 ? count - 1 : index - 1;
				const std::size_t next = (index + 1) % count;
				const float value = (
					boundary.mControlRadii[previous] +
					2.f * boundary.mControlRadii[index] +
					boundary.mControlRadii[next]
				) * 0.25f;
				smoothed[index] = std::clamp(
					value,
					boundary.mOuterMinRadius,
					boundary.mOuterMaxRadius
				);
			}
			boundary.mControlRadii = std::move(smoothed);
		}

		return boundary;
	}

	float IonStormBoundary::GetBoundaryRadius(float angleRadians) const
	{
		if (mControlRadii.empty())
		{
			return mOuterMaxRadius;
		}

		float normalizedAngle = std::fmod(angleRadians, TwoPi);
		if (normalizedAngle < 0.f)
		{
			normalizedAngle += TwoPi;
		}

		const float samplePosition = normalizedAngle /
			TwoPi * static_cast<float>(mControlRadii.size());
		const std::size_t lowerIndex = static_cast<std::size_t>(samplePosition) %
			mControlRadii.size();
		const std::size_t upperIndex = (lowerIndex + 1) % mControlRadii.size();
		const float interpolation = samplePosition -
			std::floor(samplePosition);
		const float radius = mControlRadii[lowerIndex] +
			(mControlRadii[upperIndex] - mControlRadii[lowerIndex]) *
			interpolation;
		return std::clamp(radius, mOuterMinRadius, mOuterMaxRadius);
	}

	bool IonStormBoundary::Contains(const sf::Vector2f& offset) const
	{
		const float distanceSquared = offset.x * offset.x + offset.y * offset.y;
		if (!std::isfinite(distanceSquared))
		{
			return false;
		}

		const float innerRadiusSquared = mInnerCoreRadius * mInnerCoreRadius;
		if (distanceSquared <= innerRadiusSquared)
		{
			return true;
		}

		const float distance = std::sqrt(distanceSquared);
		if (distance > mOuterMaxRadius)
		{
			return false;
		}

		const float angle = std::atan2(offset.y, offset.x);
		// Boundary points are sampled by the renderer with the same floating-point
		// interpolation. A tiny tolerance prevents an edge vertex from being
		// rejected by gameplay solely because sqrt/interpolation rounded apart.
		return distance <= GetBoundaryRadius(angle) + 0.001f;
	}

	List<sf::Vector2f> IonStormBoundary::BuildBoundaryPoints(
		std::size_t pointCount
	) const
	{
		List<sf::Vector2f> points;
		pointCount = std::max<std::size_t>(3, pointCount);
		points.reserve(pointCount);
		for (std::size_t index = 0; index < pointCount; ++index)
		{
			const float angle = TwoPi * static_cast<float>(index) /
				static_cast<float>(pointCount);
			const float radius = GetBoundaryRadius(angle);
			points.push_back({
				std::cos(angle) * radius,
				std::sin(angle) * radius
			});
		}
		return points;
	}
}
