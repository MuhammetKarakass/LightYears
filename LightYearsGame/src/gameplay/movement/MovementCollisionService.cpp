#include "gameplay/movement/MovementCollisionService.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>

namespace vega::movement
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

	sf::Vector2f ResolveTeleportDestination(const Actor& actor, const sf::Vector2f& destination)
	{
		const auto* world = actor.GetWorld();
		if (!world) return destination;
		const auto start = actor.GetActorLocation();
		const auto offset = destination - start;
		const float radius = ResolveMovingRadius(actor);
		const auto candidates = world->GetActorsInBounds(targeting::swept::SegmentBounds(start, destination, radius));
		float fraction = 1.f;
		// Each pass backs out of a destination-overlapping solid. Recheck the new
		// endpoint so overlapping/adjacent solids cannot leave the hull embedded.
		for (;;)
		{
			const auto end = start + offset * fraction;
			float safeFraction = fraction;
			for (const auto& weak : candidates)
			{
				const auto obstacle = weak.lock();
				if (!obstacle || obstacle.get() == &actor || obstacle->GetIsPendingDestroy() || !obstacle->IsPhysicsEnabled() || obstacle->GetPhysicsBodyType() != PhysicsBodyType::Static) continue;
				const float rotation = obstacle->GetActorRotation() * .01745329251994329577f;
				const float cosine = std::cos(rotation), sine = std::sin(rotation);
				for (std::size_t index = 0; index < obstacle->GetPhysicsCollisionBoxCount(); ++index)
				{
					const auto box = obstacle->GetPhysicsCollisionBox(index);
					if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f) continue;
					const auto center = obstacle->GetActorLocation() + sf::Vector2f{ cosine * box.localCenter.x - sine * box.localCenter.y, sine * box.localCenter.x + cosine * box.localCenter.y };
					const float angle = rotation + box.localRotationDegrees * .01745329251994329577f;
					float entry = 0.f;
					sf::Vector2f normal;
					if (!targeting::swept::SegmentIntersectsExpandedOrientedBox(end, end, center, box.halfExtents, angle, radius, entry, normal)) continue;
					if (targeting::swept::SegmentIntersectsExpandedOrientedBox(start, destination, center, box.halfExtents, angle, radius, entry, normal)) safeFraction = std::min(safeFraction, std::max(0.f, entry - .0005f));
				}
				if (obstacle->GetPhysicsCollisionBoxCount() == 0 && obstacle->GetPhysicsCollisionRadius() > 0.f)
				{
					const float expanded = radius + obstacle->GetPhysicsCollisionRadius();
					if (GetVectorLength(end - obstacle->GetActorLocation()) > expanded) continue;
					float entry = 0.f;
					if (targeting::swept::SegmentIntersectsExpandedBounds(start, destination, { obstacle->GetActorLocation(), {} }, expanded, entry)) safeFraction = std::min(safeFraction, std::max(0.f, entry - .0005f));
				}
			}
			if (safeFraction >= fraction || safeFraction <= 0.f) return start + offset * safeFraction;
			fraction = safeFraction;
		}
	}

	bool FindFirstStaticGeometryHit(
		const Actor& queryActor,
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd,
		float expansion,
		StaticGeometrySweepHit& outHit,
		bool respectCollisionFilters
	)
	{
		const sf::Vector2f offset = segmentEnd - segmentStart;
		const float segmentLength = GetVectorLength(offset);
		World* world = queryActor.GetWorld();
		if (!world || segmentLength <= 0.001f)
		{
			return false;
		}

		StaticGeometrySweepHit earliestHit;
		bool foundHit = false;
		const float safeExpansion = std::max(0.f, expansion);
		for (const weak_ptr<Actor>& candidateWeak : world->GetActorsInBounds(
			targeting::swept::RadiusBounds(
				(segmentStart + segmentEnd) * 0.5f,
				segmentLength * 0.5f + safeExpansion
			)
		))
		{
			const shared_ptr<Actor> candidate = candidateWeak.lock();
			if (!candidate || candidate.get() == &queryActor ||
				candidate->GetIsPendingDestroy() ||
				candidate->GetPhysicsBodyType() != PhysicsBodyType::Static)
			{
				continue;
			}

			// Movement keeps Box2D's bilateral layer/mask rule; line-of-sight
			// queries can ignore it so environment walls still occlude weapons.
			if (respectCollisionFilters &&
				(static_cast<std::uint32_t>(candidate->GetCollisionMask() & queryActor.GetCollisionLayer()) == 0 ||
					static_cast<std::uint32_t>(queryActor.GetCollisionMask() & candidate->GetCollisionLayer()) == 0))
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
					segmentStart,
					segmentEnd,
					boxCenter,
					box.halfExtents,
					actorRotationRadians +
						box.localRotationDegrees * 0.01745329251994329577f,
					safeExpansion,
					fraction,
					surfaceNormal
				) && (!foundHit || fraction < earliestHit.fraction))
				{
					foundHit = true;
					earliestHit.fraction = fraction;
					earliestHit.surfaceNormal = surfaceNormal;
				}
			}
		}

		if (!foundHit)
		{
			return false;
		}
		outHit = earliestHit;
		return true;
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
		StaticGeometrySweepHit hit;
		const bool hasHit = FindFirstStaticGeometryHit(
			movingActor,
			start,
			end,
			movingRadius,
			hit
		);
		const float earliestFraction = hasHit ? hit.fraction : 1.f;
		const sf::Vector2f impactNormal = hasHit ? hit.surfaceNormal : sf::Vector2f{};

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
