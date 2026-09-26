#include "gameplay/projectile/ProjectileReflectionService.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "framework/World.h"
#include "gameplay/projectile/ProjectileReflectionParticipant.h"
#include "gameplay/projectile/ProjectileReflectionRegistryActor.h"
#include "framework/MathUtility.h"
#include "framework/TimerManager.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		sf::Vector2f NormalizeOrFallback(
			const sf::Vector2f& value,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(value);
			return length > 0.001f ? value / length : fallback;
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

	ProjectileReflectionService::Registration::Registration(
		std::weak_ptr<ProjectileReflectionRegistryActor> registry,
		unsigned int defenderId,
		uint64_t generation
	)
		: mRegistry(std::move(registry))
		, mDefenderId(defenderId)
		, mGeneration(generation)
	{
	}

	ProjectileReflectionService::Registration::~Registration()
	{
		Reset();
	}

	ProjectileReflectionService::Registration::Registration(Registration&& other) noexcept
		: mRegistry(std::move(other.mRegistry))
		, mDefenderId(other.mDefenderId)
		, mGeneration(other.mGeneration)
	{
		other.mRegistry.reset();
		other.mDefenderId = 0u;
		other.mGeneration = 0u;
	}

	ProjectileReflectionService::Registration&
	ProjectileReflectionService::Registration::operator=(Registration&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			mRegistry = std::move(other.mRegistry);
			mDefenderId = other.mDefenderId;
			mGeneration = other.mGeneration;
			other.mRegistry.reset();
			other.mDefenderId = 0u;
			other.mGeneration = 0u;
		}
		return *this;
	}

	void ProjectileReflectionService::Registration::Reset()
	{
		if (mDefenderId == 0u)
		{
			return;
		}

		if (const std::shared_ptr<ProjectileReflectionRegistryActor> registry = mRegistry.lock())
		{
			registry->UnregisterReceiver(mDefenderId, mGeneration);
		}
		mRegistry.reset();
		mDefenderId = 0u;
		mGeneration = 0u;
	}

	ProjectileReflectionService::Registration ProjectileReflectionService::RegisterReceiver(
		Actor& defender,
		ProjectileReflectionReceiver& receiver
	)
	{
		World* const world = defender.GetWorld();
		const unsigned int defenderId = defender.GetUniqueID();
		if (!world || defenderId == 0u || defender.GetIsPendingDestroy())
		{
			return Registration{};
		}

		const std::shared_ptr<ProjectileReflectionRegistryActor> registry =
			FindOrCreateRegistryActor(*world);
		if (!registry)
		{
			return Registration{};
		}

		const uint64_t generation = registry->RegisterReceiver(defender, receiver);
		return generation == 0u
			? Registration{}
			: Registration{ registry, defenderId, generation };
	}

	bool ProjectileReflectionService::TryReflectProjectile(
		AbilityWorldActor& projectile,
		Actor& defender
	)
	{
		World* const world = defender.GetWorld();
		if (!world || projectile.GetWorld() != world ||
			projectile.GetIsPendingDestroy() || defender.GetIsPendingDestroy())
		{
			return false;
		}

		const std::shared_ptr<ProjectileReflectionRegistryActor> registry =
			FindRegistryActor(*world);
		return registry && registry->TryReflectProjectile(projectile, defender);
	}

	bool ProjectileReflectionService::TryReflectProjectileAlongPath(
		AbilityWorldActor& projectile,
		const sf::Vector2f& start,
		const sf::Vector2f& end
	)
	{
		World* const world = projectile.GetWorld();
		if (!world || projectile.GetIsPendingDestroy())
		{
			return false;
		}

		const std::shared_ptr<ProjectileReflectionRegistryActor> registry =
			FindRegistryActor(*world);
		return registry && registry->TryReflectProjectileAlongPath(projectile, start, end);
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

		const uint64_t surfaceId = surface.GetUniqueID();
		if (participant->IsSurfaceLocked(surfaceId))
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
			const uint64_t lockGeneration = participant->LockSurface(surfaceId);
			TimerManager::GetGameTimerManager().SetTimer(
				projectile.GetWeakPtr(),
				[projectileHandle = projectile.GetWeakPtr(), surfaceId, lockGeneration]() {
					const shared_ptr<Object> object = projectileHandle.lock();
					const shared_ptr<Actor> liveProjectile =
						object ? std::dynamic_pointer_cast<Actor>(object) : shared_ptr<Actor>{};
					if (liveProjectile && !liveProjectile->GetIsPendingDestroy())
					{
						if (auto* liveParticipant =
							dynamic_cast<ProjectileReflectionParticipant*>(liveProjectile.get()))
						{
							liveParticipant->UnlockSurface(surfaceId, lockGeneration);
						}
					}
				},
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

	std::shared_ptr<ProjectileReflectionRegistryActor>
	ProjectileReflectionService::FindRegistryActor(World& world)
	{
		return world.FindServiceActor<ProjectileReflectionRegistryActor>();
	}

	std::shared_ptr<ProjectileReflectionRegistryActor>
	ProjectileReflectionService::FindOrCreateRegistryActor(World& world)
	{
		if (const std::shared_ptr<ProjectileReflectionRegistryActor> registry =
			FindRegistryActor(world))
		{
			return registry;
		}
		auto registry = world.SpawnActor<ProjectileReflectionRegistryActor>().lock();
		if (!world.RegisterServiceActor(registry)) return {};
		return registry;
	}
}
