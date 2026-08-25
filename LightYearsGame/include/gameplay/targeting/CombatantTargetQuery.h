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

		// The source still determines friendly/enemy relation, while the origin
		// can belong to a moving field, projectile, or telegraph. This keeps the
		// reusable combatant query correct for actors whose effect center moves
		// independently from their owner.
		List<shared_ptr<Actor>> FindOpposingCombatants(
			World& world,
			const Actor& source,
			const sf::Vector2f& origin,
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

		// Returns the opposing combatant whose local neighborhood contains the
		// most opposing combatants. The search radius and density radius are
		// caller-owned so different mechanics can reuse the policy with different
		// spatial scales without embedding ability-specific rules here.
		shared_ptr<Actor> FindDensestOpposingCombatant(
			World& world,
			const Actor& source,
			const sf::Vector2f& origin,
			float searchRadius,
			float densityRadius
		);
	}
}
