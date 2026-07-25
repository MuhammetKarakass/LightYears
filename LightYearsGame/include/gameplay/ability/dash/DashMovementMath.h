#pragma once

#include <algorithm>
#include <cmath>
#include <SFML/System/Vector2.hpp>

namespace ly::DashMovementMath
{
	inline constexpr float RatingDiminishingScale = 20.f;
	inline constexpr float MaximumDistanceBonus = 0.50f;

	// Both resolved movement ratings contribute equally; the asymptotic curve
	// keeps high-stat builds from turning a dash into an arena traversal.
	inline float ResolveDistance(float baseDistance, float horizontalRating, float verticalRating)
	{
		const float combinedRating = 0.5f * (
			std::max(0.f, horizontalRating) + std::max(0.f, verticalRating)
		);
		const float saturation = 1.f - std::exp(-combinedRating / RatingDiminishingScale);
		const float distanceMultiplier = 1.f + MaximumDistanceBonus * std::clamp(saturation, 0.f, 1.f);
		return std::max(0.f, baseDistance) * distanceMultiplier;
	}

	inline sf::Vector2f ResolveVelocity(
		sf::Vector2f direction,
		float resolvedDistance,
		float duration,
		const sf::Vector2f& preservedVelocity)
	{
		const float directionLength = std::sqrt(
			direction.x * direction.x + direction.y * direction.y
		);
		if (directionLength <= 0.001f || resolvedDistance <= 0.f || duration <= 0.f)
		{
			return preservedVelocity;
		}

		direction /= directionLength;
		return preservedVelocity + direction * (resolvedDistance / duration);
	}
}
