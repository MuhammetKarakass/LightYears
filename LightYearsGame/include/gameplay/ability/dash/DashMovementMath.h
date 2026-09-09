#pragma once

#include "gameplay/movement/MovementBurstMath.h"

namespace ly::DashMovementMath
{
	inline constexpr float RatingDiminishingScale = movement::MovementBurstMath::RatingDiminishingScale;
	inline constexpr float MaximumDistanceBonus = movement::MovementBurstMath::MaximumDistanceBonus;

	inline float ResolveDistance(float baseDistance, float horizontalRating, float verticalRating)
	{
		return movement::MovementBurstMath::ResolveDistance(
			baseDistance,
			horizontalRating,
			verticalRating
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
