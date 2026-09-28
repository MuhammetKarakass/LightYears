#pragma once

#include "gameplay/movement/MovementBurstMath.h"

namespace ly::DashMovementMath
{
	inline constexpr float RatingDiminishingScale = movement::MovementBurstMath::RatingDiminishingScale;
	inline constexpr float MaximumDistanceBonus = movement::MovementBurstMath::MaximumDistanceBonus;
	inline constexpr float MoveSpeedRatingPercent = 100.f;

	inline float ResolveDistance(float baseDistance, float horizontalRating, float verticalRating)
	{
		return movement::MovementBurstMath::ResolveDistance(
			baseDistance,
			horizontalRating,
			verticalRating
		);
	}

	inline float ResolveDistanceFromMoveSpeed(
		float baseDistance,
		sf::Vector2f direction,
		float horizontalRating,
		float verticalRating,
		float moveSpeedScale)
	{
		const float horizontalWeight = std::isfinite(direction.x) ? std::abs(direction.x) : 0.f;
		const float verticalWeight = std::isfinite(direction.y) ? std::abs(direction.y) : 0.f;
		const float totalWeight = horizontalWeight + verticalWeight;
		const float safeHorizontalRating = std::isfinite(horizontalRating) ? std::max(0.f, horizontalRating) : 0.f;
		const float safeVerticalRating = std::isfinite(verticalRating) ? std::max(0.f, verticalRating) : 0.f;
		const float projectedRating = totalWeight > 0.f
			? (horizontalWeight * safeHorizontalRating + verticalWeight * safeVerticalRating) / totalWeight
			: 0.5f * (safeHorizontalRating + safeVerticalRating);
		const float safeScale = std::isfinite(moveSpeedScale) ? std::max(0.f, moveSpeedScale) : 0.f;
		return std::max(0.f, baseDistance) * (
			1.f + projectedRating / MoveSpeedRatingPercent * safeScale
		);
	}

	inline sf::Vector2f ResolveVelocity(
		sf::Vector2f direction,
		float resolvedDistance,
		float duration,
		const sf::Vector2f& preservedVelocity)
	{
		return movement::MovementBurstMath::ResolveVelocity(
			direction,
			resolvedDistance,
			duration,
			preservedVelocity
		);
	}
}
