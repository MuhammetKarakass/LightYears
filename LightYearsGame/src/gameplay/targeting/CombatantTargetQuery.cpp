#include "gameplay/targeting/CombatantTargetQuery.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/combat/Combatant.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/targeting/SectorGeometry.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

	namespace vega::targeting
{
	List<shared_ptr<Actor>> FindOpposingCombatantsInSector(World& world, const Actor& source, const sf::Vector2f& origin, const sf::Vector2f& direction, float radius, float halfAngleRadians)
	{
		List<shared_ptr<Actor>> result;
		TargetingQuery query;
		query.source = &source;
		query.origin = origin;
		query.requiredTargetLayers = ResolveOpposingLayer(source);
		query.requireCollisionCompatibility = true;
		for (const auto& weak : world.GetActorsInBounds(swept::RadiusBounds(origin, radius)))
		{
			const auto actor = weak.lock();
			if (!actor || actor->GetIsPendingDestroy() || !IsOpposingTarget(source, *actor) || !dynamic_cast<Combatant*>(actor.get())) continue;
			if (const auto* ship = dynamic_cast<const SpaceShip*>(actor.get()); ship && ship->GetHealthComponent().GetHealth() <= 0.f) continue;
			const auto bounds = actor->GetActorGlobalBounds();
			const float targetRadius = std::max(0.f, actor->GetPhysicsCollisionRadius());
			query.range = radius + targetRadius;
			if (!AutoTargeting::IsValidTarget(*actor, query)) continue;
			const auto center = bounds.size.x > 0.f && bounds.size.y > 0.f ? bounds.position + bounds.size * .5f : actor->GetActorLocation();
			if (CircleOverlapsSector(center, targetRadius, origin, direction, radius, halfAngleRadians)) result.push_back(actor);
		}
		return result;
	}

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
		const vega::CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(source);
		if (opposingLayer == vega::CollisionLayer::None)
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
		const vega::CollisionLayer opposingLayer = targeting::ResolveOpposingLayer(source);
		if (opposingLayer == vega::CollisionLayer::None)
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
