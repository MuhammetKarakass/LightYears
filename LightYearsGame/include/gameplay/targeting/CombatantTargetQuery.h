#pragma once

#include "framework/Core.h"

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class Actor;
	class World;

	namespace targeting
	{
		// Finds live opposing Combatant actors inside a configurable targeting
		// query. The Combatant check deliberately excludes projectiles, pickups,
		// fields and other non-ship actors without naming any ability family.
		List<shared_ptr<Actor>> FindOpposingCombatants(
			World& world,
			const Actor& source,
			float range
		);

		// Selects a damaged opposing ship inside a directional cone. This is a
		// reusable chase query; it does not know which ability requested it.
		shared_ptr<Actor> FindBestOpposingDamagedShip(
			World& world,
			const Actor& source,
			const sf::Vector2f& movementDirection,
			float range,
			float directionThreshold
		);
	}
}
