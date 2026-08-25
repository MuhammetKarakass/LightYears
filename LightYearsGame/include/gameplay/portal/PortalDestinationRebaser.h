#pragma once

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <cmath>

namespace ly
{
	// Opt-in contract for a delivery projectile that owns a future world
	// destination (field, capture volume, impact point). AbilityWorldActor
	// performs every physical portal operation; this contract only rebases that
	// future destination after a completed transfer.
	class PortalDestinationRebaser
	{
	public:
		virtual ~PortalDestinationRebaser() = default;
		virtual void RebasePortalDestination(
			const sf::Vector2f& exitLocation
		) = 0;
	};

	namespace portal
	{
		inline sf::Vector2f RebaseForwardDestination(
			const sf::Vector2f& exitLocation,
			const sf::Vector2f& forwardDirection,
			float remainingDistance
		)
		{
			const float length = std::sqrt(
				forwardDirection.x * forwardDirection.x +
				forwardDirection.y * forwardDirection.y
			);
			const sf::Vector2f direction = length > 0.001f
				? forwardDirection / length
				: sf::Vector2f{ 0.f, -1.f };
			return exitLocation + direction * std::max(0.f, remainingDistance);
		}
	}
}
