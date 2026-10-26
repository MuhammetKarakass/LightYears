#pragma once

#include <SFML/System/Vector2.hpp>

namespace vega
{
	class Actor;

	namespace movement
	{
		// Only the destination is tested. Obstacles crossed en route do not block a teleport.
		sf::Vector2f ResolveTeleportDestination(const Actor& actor, const sf::Vector2f& destination);
		struct StaticGeometrySweepHit
		{
			float fraction = 1.f;
			sf::Vector2f surfaceNormal{};
		};

		// Finds the first static box hit along an arbitrary segment. Callers may
		// ignore collision masks when static geometry should block line of sight.
		bool FindFirstStaticGeometryHit(
			const Actor& queryActor,
			const sf::Vector2f& segmentStart,
			const sf::Vector2f& segmentEnd,
			float expansion,
			StaticGeometrySweepHit& outHit,
			bool respectCollisionFilters = true
		);

		// Resolves movement authored by gameplay code against static box geometry.
		// Actor locations are game-authoritative, so Box2D contact callbacks alone
		// cannot prevent a ship from being moved through a wall on the next frame.
		// Keeping that correction here lets input, dash and external impulses obey
		// the same physical-obstacle rule without coupling them to a specific ability.
		sf::Vector2f ConstrainMovementAgainstStaticGeometry(
			Actor& movingActor,
			const sf::Vector2f& requestedOffset
		);
	}
}
