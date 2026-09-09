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
		Actor& movingActor,
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
		sf::Vector2f impactNormal{};

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

			// Keep the authoritative movement sweep aligned with the physics
			// system's bilateral layer/mask rule. Lance Drive is Environment ->
			// Enemy, so it blocks enemies while its Player caster remains free.
			if (static_cast<uint8_t>(candidate->GetCollisionMask() & movingActor.GetCollisionLayer()) == 0 ||
				static_cast<uint8_t>(movingActor.GetCollisionMask() & candidate->GetCollisionLayer()) == 0)
			{
				continue;
			}

			const float actorRotationRadians =
				candidate->GetActorRotation() * 0.01745329251994329577f;
			const float cosine = std::cos(actorRotationRadians);
			const float sine = std::sin(actorRotationRadians);
			for (std::size_t boxIndex = 0;
				boxIndex < candidate->GetPhysicsCollisionBoxCount();
				++boxIndex)
			{
				const PhysicsCollisionBox box = candidate->GetPhysicsCollisionBox(boxIndex);
				if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f)
				{
					continue;
				}
				const sf::Vector2f boxCenter = candidate->GetActorLocation() + sf::Vector2f{
					cosine * box.localCenter.x - sine * box.localCenter.y,
					sine * box.localCenter.x + cosine * box.localCenter.y
				};
				float fraction = 1.f;
				sf::Vector2f surfaceNormal;
				if (targeting::swept::SegmentIntersectsExpandedOrientedBox(
					start,
					end,
					boxCenter,
					box.halfExtents,
					actorRotationRadians +
						box.localRotationDegrees * 0.01745329251994329577f,
					movingRadius,
					fraction,
					surfaceNormal
				))
				{
					if (fraction < earliestFraction)
					{
						earliestFraction = fraction;
						impactNormal = surfaceNormal;
					}
				}
			}
		}

		// Stay just before the contact plane. This prevents resting movement from
		// alternating between the two sides because of floating-point rounding.
		const float safeFraction = earliestFraction < 1.f
			? std::max(0.f, earliestFraction - 0.0005f)
			: 1.f;
		if (safeFraction < 1.f && GetVectorLength(impactNormal) > 0.001f)
		{
			// Actor locations are game-authoritative, so a sweep must also resolve
			// velocity. Remove and slightly reflect the incoming normal component;
			// this gives static ability walls a genuine angle-dependent deflection
			// instead of letting a ship keep pushing through them every tick.
			const float inwardSpeed =
				movingActor.GetVelocity().x * impactNormal.x +
				movingActor.GetVelocity().y * impactNormal.y;
			if (inwardSpeed < 0.f)
			{
				constexpr float WallRestitution = 0.35f;
				movingActor.SetVelocity(
					movingActor.GetVelocity() -
					impactNormal * ((1.f + WallRestitution) * inwardSpeed)
				);
			}
		}
		return requestedOffset * safeFraction;
	}
}
