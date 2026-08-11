#include "gameplay/ability/nullPulse/NullPulseTargetQuery.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/AutoTargeting.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		float DistanceSquared(const sf::Vector2f& left, const sf::Vector2f& right)
		{
			const sf::Vector2f delta = left - right;
			return delta.x * delta.x + delta.y * delta.y;
		}

		CollisionLayer ResolveEnemyLayer(const Actor& source)
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

	namespace NullPulseTargetQuery
	{
		List<shared_ptr<AbilityWorldActor>> FindClearableProjectiles(
			World& world,
			const Actor& source,
			float radius
		)
		{
			List<shared_ptr<AbilityWorldActor>> result;
			const float safeRadius = std::max(0.f, radius);
			const float radiusSquared = safeRadius * safeRadius;
			Set<Actor*> seenActors;

			for (const weak_ptr<Actor>& actorWeak : world.GetActorsByType<Actor>())
			{
				const shared_ptr<Actor> actor = actorWeak.lock();
				if (!actor || actor.get() == &source ||
					!seenActors.insert(actor.get()).second ||
					actor->GetIsPendingDestroy())
				{
					continue;
				}

				const shared_ptr<AbilityWorldActor> abilityActor =
					std::dynamic_pointer_cast<AbilityWorldActor>(actor);
				if (!abilityActor || !abilityActor->IsProjectileActor() ||
					DistanceSquared(abilityActor->GetActorLocation(), source.GetActorLocation()) > radiusSquared)
				{
					continue;
				}
				result.push_back(abilityActor);
			}
			return result;
		}

		List<shared_ptr<Actor>> FindEnemyTargets(
			World& world,
			const Actor& source,
			float radius
		)
		{
			List<shared_ptr<Actor>> result;
			const CollisionLayer enemyLayer = ResolveEnemyLayer(source);
			if (enemyLayer == CollisionLayer::None)
			{
				return result;
			}

			targeting::TargetingQuery query;
			query.source = &source;
			query.origin = source.GetActorLocation();
			query.range = std::max(0.f, radius);
			query.requiredTargetLayers = enemyLayer;
			query.requireCollisionCompatibility = true;
			query.filter = [](
				const Actor*,
				const Actor& candidate,
				const targeting::TargetingCandidate&)
			{
				return dynamic_cast<const Combatant*>(&candidate) != nullptr;
			};

			for (const targeting::TargetingCandidate& candidate :
				targeting::AutoTargeting::FindTargets(world, query))
			{
				if (candidate.actor)
				{
					result.push_back(candidate.actor);
				}
			}
			return result;
		}
	}
}
