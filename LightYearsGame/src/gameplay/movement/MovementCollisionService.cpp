#include "gameplay/movement/MovementCollisionService.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>

namespace ly::movement
{
	namespace
	{
		float ResolveMovingRadius(const Actor& actor)
		{
			const sf::Vector2f explicitHalfExtents = actor.GetPhysicsCollisionBoxHalfExtents();
			if (explicitHalfExtents.x > 0.f || explicitHalfExtents.y > 0.f)
			{
				return std::max(explicitHalfExtents.x, explicitHalfExtents.y);
			}
			if (actor.GetPhysicsCollisionRadius() > 0.f)
			{
				return actor.GetPhysicsCollisionRadius();
			}
			const sf::FloatRect bounds = actor.GetActorGlobalBounds();
			return std::max(0.f, std::min(bounds.size.x, bounds.size.y) * 0.5f);
		}
	}

	sf::Vector2f ConstrainMovementAgainstStaticGeometry(
		const Actor& movingActor,
		const sf::Vector2f& requestedOffset
	)
	{
		if (GetVectorLength(requestedOffset) <= 0.001f)
		{
			return requestedOffset;
		}

		World* world = movingActor.GetWorld();
		if (!world)
		{
			return requestedOffset;
		}

		const sf::Vector2f start = movingActor.GetActorLocation();
		const sf::Vector2f end = start + requestedOffset;
		const float movingRadius = ResolveMovingRadius(movingActor);
		float earliestFraction = 1.f;

		for (const weak_ptr<Actor>& candidateWeak : world->GetActorsInBounds(
			targeting::swept::RadiusBounds(
				(start + end) * 0.5f,
				GetVectorLength(requestedOffset) * 0.5f + movingRadius
			)
		))
		{
			const shared_ptr<Actor> candidate = candidateWeak.lock();
			if (!candidate || candidate.get() == &movingActor ||
				candidate->GetIsPendingDestroy() ||
				candidate->GetPhysicsBodyType() != PhysicsBodyType::Static)
			{
				continue;
			}

			const sf::Vector2f halfExtents = candidate->GetPhysicsCollisionBoxHalfExtents();
			if (halfExtents.x <= 0.f && halfExtents.y <= 0.f)
			{
				continue;
			}

			float fraction = 1.f;
			sf::Vector2f ignoredNormal;
			if (targeting::swept::SegmentIntersectsExpandedOrientedBox(
				start,
				end,
				candidate->GetActorLocation(),
				halfExtents,
				candidate->GetActorRotation() * 0.01745329251994329577f,
				movingRadius,
				fraction,
				ignoredNormal
			))
			{
				earliestFraction = std::min(earliestFraction, fraction);
			}
		}

		// Stay just before the contact plane. This prevents resting movement from
		// alternating between the two sides because of floating-point rounding.
		const float safeFraction = earliestFraction < 1.f
			? std::max(0.f, earliestFraction - 0.0005f)
			: 1.f;
		return requestedOffset * safeFraction;
	}
}
