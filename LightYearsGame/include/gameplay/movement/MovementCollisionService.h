#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class Actor;

	namespace movement
	{
		// Resolves movement authored by gameplay code against static box geometry.
		// Actor locations are game-authoritative, so Box2D contact callbacks alone
		// cannot prevent a ship from being moved through a wall on the next frame.
		// Keeping that correction here lets input, dash and external impulses obey
		// the same physical-obstacle rule without coupling them to a specific ability.
		sf::Vector2f ConstrainMovementAgainstStaticGeometry(
			const Actor& movingActor,
			const sf::Vector2f& requestedOffset
		);
	}
}
