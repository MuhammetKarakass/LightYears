#include "gameplay/targeting/CombatantTargetQuery.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/AutoTargeting.h"

#include <algorithm>
#include <cmath>

namespace ly::targeting
{
	namespace
	{
		CollisionLayer ResolveOpposingLayer(const Actor& source)
		{
			if (source.GetCollisionLayer() == CollisionLayer::Player)
			{
				return CollisionLayer::Enemy;
			}
			if (source.GetCollisionLayer() == CollisionLayer::Enemy)
			{
				return CollisionLayer::Player;
			}
			return CollisionLayer::None;
		}
	}

	List<shared_ptr<Actor>> FindOpposingCombatants(
		World& world,
		const Actor& source,
		float range
	)
	{
		List<shared_ptr<Actor>> result;
		const CollisionLayer opposingLayer = ResolveOpposingLayer(source);
		if (opposingLayer == CollisionLayer::None)
		{
			return result;
		}

		TargetingQuery query;
		query.source = &source;
		query.origin = source.GetActorLocation();
		query.range = std::max(0.f, range);
		query.requiredTargetLayers = opposingLayer;
		query.requireCollisionCompatibility = true;
		query.filter = [](
			const Actor*,
			const Actor& candidate,
			const TargetingCandidate&)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};

		for (const TargetingCandidate& candidate : AutoTargeting::FindTargets(world, query))
		{
			if (candidate.actor)
			{
				result.push_back(candidate.actor);
			}
		}
		return result;
	}

	shared_ptr<Actor> FindBestOpposingDamagedShip(
		World& world,
		const Actor& source,
		const sf::Vector2f& movementDirection,
		float range,
		float directionThreshold
	)
	{
		const float directionLength = std::sqrt(
			movementDirection.x * movementDirection.x +
			movementDirection.y * movementDirection.y
		);
		if (directionLength <= 0.001f)
		{
			return {};
		}

		const float safeThreshold = std::clamp(directionThreshold, -1.f, 1.f);
		const CollisionLayer opposingLayer = ResolveOpposingLayer(source);
		if (opposingLayer == CollisionLayer::None)
		{
			return {};
		}
		TargetingQuery query;
		query.source = &source;
		query.origin = source.GetActorLocation();
		query.range = std::max(0.f, range);
		query.shape = TargetingShape::Cone;
		query.direction = movementDirection;
		query.coneHalfAngleRadians = std::acos(safeThreshold);
		query.requiredTargetLayers = opposingLayer;
		query.requireCollisionCompatibility = true;
		query.comparator = [](
			const TargetingCandidate& left,
			const TargetingCandidate& right)
		{
			if (left.angleRadians != right.angleRadians)
			{
				return left.angleRadians < right.angleRadians;
			}
			return left.distanceSquared < right.distanceSquared;
		};
		query.filter = [](
			const Actor*,
			const Actor& candidate,
			const TargetingCandidate&)
		{
			const SpaceShip* ship = dynamic_cast<const SpaceShip*>(&candidate);
			return ship &&
				ship->GetHealthComponent().GetHealth() > 0.f &&
				ship->GetHealthComponent().GetHealth() <
					ship->GetHealthComponent().GetMaxHealth();
		};

		return AutoTargeting::FindTarget(world, query).lock();
	}
}
