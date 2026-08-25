#include "gameplay/projectile/ProjectileReflectionService.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionParticipant.h"
#include "framework/MathUtility.h"
#include "framework/TimerManager.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace ly
{
	namespace
	{
		std::unordered_map<Actor*, ProjectileReflectionReceiver*>& Receivers()
		{
			static std::unordered_map<Actor*, ProjectileReflectionReceiver*> receivers;
			return receivers;
		}

		struct SurfaceLockKey
		{
			uint64_t projectileId = 0;
			uint64_t surfaceId = 0;

			bool operator==(const SurfaceLockKey& other) const
			{
				return projectileId == other.projectileId && surfaceId == other.surfaceId;
			}
		};

		struct SurfaceLockKeyHash
		{
			std::size_t operator()(const SurfaceLockKey& key) const
			{
				return std::hash<uint64_t>{}(key.projectileId) ^
					(std::hash<uint64_t>{}(key.surfaceId) << 1);
			}
		};

		std::unordered_map<SurfaceLockKey, bool, SurfaceLockKeyHash>& SurfaceLocks()
		{
			static std::unordered_map<SurfaceLockKey, bool, SurfaceLockKeyHash> locks;
			return locks;
		}

		sf::Vector2f NormalizeOrFallback(
			const sf::Vector2f& value,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(value);
			return length > 0.001f ? value / length : fallback;
		}

		float DistanceSquaredToSegment(
			const sf::Vector2f& point,
			const sf::Vector2f& start,
			const sf::Vector2f& end
		)
		{
			const sf::Vector2f segment = end - start;
			const float segmentLengthSquared =
				segment.x * segment.x + segment.y * segment.y;
			if (segmentLengthSquared <= 0.000001f)
			{
				const sf::Vector2f offset = point - start;
				return offset.x * offset.x + offset.y * offset.y;
			}
			const sf::Vector2f pointOffset = point - start;
			const float fraction = std::clamp(
				(pointOffset.x * segment.x + pointOffset.y * segment.y) /
					segmentLengthSquared,
				0.f,
				1.f
			);
			const sf::Vector2f offset = point - (start + segment * fraction);
			return offset.x * offset.x + offset.y * offset.y;
		}

		sf::Vector2f ResolveOverlapSurfaceNormal(
			const Actor& projectile,
			const Actor& surface
		)
		{
			const sf::Vector2f incomingDirection = NormalizeOrFallback(
				projectile.GetVelocity(),
				projectile.GetActorForwardDirection()
			);
			const sf::Vector2f halfExtents = surface.GetPhysicsCollisionBoxHalfExtents();
			if (halfExtents.x <= 0.f || halfExtents.y <= 0.f)
			{
				return -incomingDirection;
			}

			const float radians = surface.GetActorRotation() * 0.01745329251994329577f;
			const sf::Vector2f localXAxis{ std::cos(radians), std::sin(radians) };
			const sf::Vector2f localYAxis{ -std::sin(radians), std::cos(radians) };
			const float alongX = incomingDirection.x * localXAxis.x +
				incomingDirection.y * localXAxis.y;
			const float alongY = incomingDirection.x * localYAxis.x +
				incomingDirection.y * localYAxis.y;
			const float normalizedX = std::abs(alongX) / halfExtents.x;
			const float normalizedY = std::abs(alongY) / halfExtents.y;
			return normalizedX > normalizedY
				? localXAxis * (alongX >= 0.f ? -1.f : 1.f)
				: localYAxis * (alongY >= 0.f ? -1.f : 1.f);
		}
	}

	bool ProjectileReflectionService::RegisterReceiver(
		Actor& defender,
		ProjectileReflectionReceiver& receiver
	)
	{
		auto& receivers = Receivers();
		const auto [iterator, inserted] = receivers.emplace(&defender, &receiver);
		return inserted || iterator->second == &receiver;
	}

	void ProjectileReflectionService::UnregisterReceiver(
		Actor& defender,
		const ProjectileReflectionReceiver& receiver
	)
	{
		auto& receivers = Receivers();
		const auto iterator = receivers.find(&defender);
		if (iterator != receivers.end() && iterator->second == &receiver)
		{
			receivers.erase(iterator);
		}
	}

	bool ProjectileReflectionService::TryReflectProjectile(
		AbilityWorldActor& projectile,
		Actor& defender
	)
	{
		const auto iterator = Receivers().find(&defender);
		return iterator != Receivers().end() && iterator->second &&
			iterator->second->TryReflectIncomingProjectile(projectile, defender);
	}

	bool ProjectileReflectionService::TryReflectProjectileAlongPath(
		AbilityWorldActor& projectile,
		const sf::Vector2f& start,
		const sf::Vector2f& end
	)
	{
		for (const auto& [defender, receiver] : Receivers())
		{
			if (!defender || !receiver || defender->GetIsPendingDestroy() ||
				defender->GetWorld() != projectile.GetWorld())
			{
				continue;
			}

			const float radius = std::max(0.1f, projectile.GetPhysicsCollisionRadius()) +
				std::max(0.1f, defender->GetPhysicsCollisionRadius());
			if (DistanceSquaredToSegment(defender->GetActorLocation(), start, end) <=
				radius * radius &&
				receiver->TryReflectIncomingProjectile(projectile, *defender))
			{
				return true;
			}
		}
		return false;
	}

	bool ProjectileReflectionService::TryReflectFromSurface(
		Actor& projectile,
		Actor& surface,
		const ProjectileReflectionSurfaceHit& hit
	)
	{
		auto* participant = dynamic_cast<ProjectileReflectionParticipant*>(&projectile);
		auto* reflectionSurface = dynamic_cast<ProjectileReflectionSurface*>(&surface);
		if (!participant || !participant->CanBeReflected() || !reflectionSurface)
		{
			return false;
		}

		const SurfaceLockKey lockKey{
			projectile.GetUniqueID(),
			surface.GetUniqueID()
		};
		if (SurfaceLocks().find(lockKey) != SurfaceLocks().end())
		{
			return false;
		}

		ProjectileReflectionSurfaceResponse response;
		if (!reflectionSurface->BuildProjectileReflectionResponse(projectile, response) ||
			!response.newOwner)
		{
			return false;
		}

		const sf::Vector2f incomingDirection = NormalizeOrFallback(
			projectile.GetVelocity(),
			projectile.GetActorForwardDirection()
		);
		const sf::Vector2f normal = NormalizeOrFallback(
			hit.surfaceNormal,
			-incomingDirection
		);
		const float projection = incomingDirection.x * normal.x +
			incomingDirection.y * normal.y;
		const sf::Vector2f reflectedDirection = incomingDirection - normal * (2.f * projection);
		const ProjectileReflectionRequest request{
			*response.newOwner,
			NormalizeOrFallback(reflectedDirection, -incomingDirection),
			std::max(0.f, response.damageMultiplier),
			response.allowSameOwnerReflection
		};
		if (!participant->TryReflectProjectile(request))
		{
			return false;
		}

		// A reflected projectile must leave the expanded collision surface before
		// the next frame. Otherwise the same surface is intentionally lock-gated,
		// then the family can mistake that second contact for an ordinary impact
		// and destroy the projectile. The offset is based on its collision radius
		// so every projectile family uses one stable separation rule.
		const float separation = std::max(
			0.5f,
			std::max(0.f, projectile.GetPhysicsCollisionRadius()) + 0.5f
		);
		projectile.SetActorLocation(hit.impactLocation + normal * separation);

		const float lockDuration = std::max(0.f, response.sameSurfaceLockDuration);
		if (lockDuration > 0.f)
		{
			SurfaceLocks().emplace(lockKey, true);
			TimerManager::GetGameTimerManager().SetTimer(
				projectile.GetWeakPtr(),
				[lockKey]() { SurfaceLocks().erase(lockKey); },
				lockDuration,
				false
			);
		}
		return true;
	}

	bool ProjectileReflectionService::TryReflectFromSurfaceOverlap(
		Actor& projectile,
		Actor& surface
	)
	{
		return TryReflectFromSurface(
			projectile,
			surface,
			{ projectile.GetActorLocation(), ResolveOverlapSurfaceNormal(projectile, surface) }
		);
	}
}
