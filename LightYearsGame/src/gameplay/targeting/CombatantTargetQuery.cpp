#include "gameplay/targeting/CombatantTargetQuery.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"

#include <algorithm>
#include <cmath>
#include <limits>

	namespace ly::targeting
{
	List<shared_ptr<Actor>> FindOpposingCombatants(
		World& world,
		const Actor& source,
		float range
	)
	{
		return FindOpposingCombatants(
			world,
			source,
			range,
			true
		);
	}

	List<shared_ptr<Actor>> FindOpposingCombatants(
		World& world,
		const Actor& source,
		float range,
		bool requireCollisionCompatibility
	)
	{
		return FindOpposingCombatants(
			world,
			source,
			source.GetActorLocation(),
			range,
			requireCollisionCompatibility
		);
	}

	List<shared_ptr<Actor>> FindOpposingCombatants(
		World& world,
		const Actor& source,
		const sf::Vector2f& origin,
		float range
	)
	{
		return FindOpposingCombatants(
			world,
			source,
			origin,
			range,
			true
		);
	}

	List<shared_ptr<Actor>> FindOpposingCombatants(
		World& world,
		const Actor& source,
		const sf::Vector2f& origin,
		float range,
		bool requireCollisionCompatibility
	)
	{
		List<shared_ptr<Actor>> result;
		const CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(source);
		if (opposingLayer == CollisionLayer::None)
		{
			return result;
		}

		TargetingQuery query;
		query.source = &source;
		query.origin = origin;
		query.range = std::max(0.f, range);
		query.requiredTargetLayers = opposingLayer;
		query.requireCollisionCompatibility = requireCollisionCompatibility;
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
		const CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(source);
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

	shared_ptr<Actor> FindDensestOpposingCombatant(
		World& world,
		const Actor& source,
		const sf::Vector2f& origin,
		float searchRadius,
		float densityRadius
	)
	{
		const List<shared_ptr<Actor>> candidates = FindOpposingCombatants(
			world,
			source,
			origin,
			std::max(0.f, searchRadius)
		);
		if (candidates.empty())
		{
			return {};
		}

		const float safeDensityRadius = std::max(0.f, densityRadius);
		const float densityRadiusSquared = safeDensityRadius * safeDensityRadius;
		shared_ptr<Actor> best;
		std::size_t bestDensity = 0;
		float bestDistanceSquared = std::numeric_limits<float>::max();
		for (const shared_ptr<Actor>& candidate : candidates)
		{
			if (!candidate || candidate->GetIsPendingDestroy())
			{
				continue;
			}

			std::size_t density = 0;
			for (const shared_ptr<Actor>& neighbor : candidates)
			{
				if (!neighbor || neighbor->GetIsPendingDestroy())
				{
					continue;
				}
				const sf::Vector2f delta =
					neighbor->GetActorLocation() - candidate->GetActorLocation();
				const float distanceSquared =
					delta.x * delta.x + delta.y * delta.y;
				if (distanceSquared <= densityRadiusSquared)
				{
					++density;
				}
			}

			const sf::Vector2f originDelta =
				candidate->GetActorLocation() - origin;
			const float distanceSquared =
				originDelta.x * originDelta.x + originDelta.y * originDelta.y;
			if (!best || density > bestDensity ||
				(density == bestDensity && distanceSquared < bestDistanceSquared))
			{
				best = candidate;
				bestDensity = density;
				bestDistanceSquared = distanceSquared;
			}
		}
		return best;
	}
}
