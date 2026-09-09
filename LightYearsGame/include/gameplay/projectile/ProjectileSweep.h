#pragma once

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>

namespace ly::projectile
{
	struct SweptContact
	{
		// Contacts are consumed in the current actor tick, before World can
		// reclaim pending actors. A raw pointer avoids shared-pointer churn for
		// every broadphase candidate in projectile-heavy frames.
		Actor* actor = nullptr;
		float fraction = 0.f;
		sf::Vector2f impactLocation{};
		sf::Vector2f surfaceNormal{};
		bool hasSurfaceNormal = false;
	};

	inline List<SweptContact> FindSweptContacts(
		Actor& projectileActor,
		const sf::Vector2f& startLocation,
		const sf::Vector2f& endLocation,
		float collisionRadius
	)
	{
		List<SweptContact> contacts;
		World* world = projectileActor.GetWorld();
		if (!world)
		{
			return contacts;
		}

		const float safeRadius = std::max(0.f, collisionRadius);
		world->ForEachActorInBounds(
			targeting::swept::SegmentBounds(
				startLocation,
				endLocation,
				safeRadius
			),
			[&](Actor& candidate)
		{
			if (&candidate == &projectileActor || candidate.GetIsPendingDestroy() ||
				!projectileActor.CanCollideWith(&candidate) ||
				!candidate.CanCollideWith(&projectileActor))
			{
				return;
			}

			float fraction = 0.f;
			sf::Vector2f surfaceNormal{};
			bool hasSurfaceNormal = false;
			const sf::Vector2f boxHalfExtents =
				candidate.GetPhysicsCollisionBoxHalfExtents();
			const bool hasExplicitBox = boxHalfExtents.x > 0.f &&
				boxHalfExtents.y > 0.f;
			const bool intersects = hasExplicitBox
				? targeting::swept::SegmentIntersectsExpandedOrientedBox(
					startLocation,
					endLocation,
					candidate.GetActorLocation(),
					boxHalfExtents,
					candidate.GetActorRotation() * 0.01745329251994329577f,
					safeRadius,
					fraction,
					surfaceNormal
				)
				: targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate.GetActorGlobalBounds(),
					safeRadius
				);
			if (!intersects)
			{
				return;
			}
			if (!hasExplicitBox)
			{
				fraction = targeting::swept::SegmentProjectionFraction(
					candidate.GetActorLocation(), startLocation, endLocation
				);
			}
			hasSurfaceNormal = hasExplicitBox;
			contacts.push_back(SweptContact{
				&candidate,
				fraction,
				startLocation + (endLocation - startLocation) * fraction,
				surfaceNormal,
				hasSurfaceNormal
			});
			}
		);
		// std::sort is in-place; stable_sort may allocate an auxiliary buffer for
		// a multi-hit projectile every frame.
		std::sort(
			contacts.begin(),
			contacts.end(),
			[](const SweptContact& left, const SweptContact& right)
			{
				return left.fraction < right.fraction;
			}
		);
		return contacts;
	}
}
