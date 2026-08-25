#include "gameplay/ability/nullPulse/NullPulseTargetQuery.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/SweptGeometry.h"

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

			for (const weak_ptr<Actor>& actorWeak : world.GetActorsInBounds(
				targeting::swept::RadiusBounds(source.GetActorLocation(), safeRadius)
			))
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
			return targeting::FindOpposingCombatants(world, source, radius);
		}
	}
}
