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
		struct ReflectionEntry
		{
			// Weak identity. The key is the monotonic Object id and the entry only ever
			// reaches the actor through this handle, so a recycled address can never be
			// mistaken for the actor that registered.
			weak_ptr<Actor> defender;
			ProjectileReflectionReceiver* receiver = nullptr;
			uint64_t generation = 0u;
		};

		std::unordered_map<unsigned int, ReflectionEntry>& ReflectionEntries()
		{
			static std::unordered_map<unsigned int, ReflectionEntry> entries;
			return entries;
		}

		uint64_t NextReflectionGeneration()
		{
			static uint64_t generation = 0u;
			return ++generation;
		}

		weak_ptr<Actor> MakeWeakActorHandle(Actor& actor)
		{
			const shared_ptr<Object> object = actor.GetWeakPtr().lock();
			return object ? std::dynamic_pointer_cast<Actor>(object) : weak_ptr<Actor>{};
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

	ProjectileReflectionService::Registration::Registration(
		unsigned int defenderId,
		uint64_t generation
	)
		: mDefenderId(defenderId)
		, mGeneration(generation)
	{
	}

	ProjectileReflectionService::Registration::~Registration()
	{
		Reset();
	}

	ProjectileReflectionService::Registration::Registration(Registration&& other) noexcept
		: mDefenderId(other.mDefenderId)
		, mGeneration(other.mGeneration)
	{
		other.mDefenderId = 0u;
		other.mGeneration = 0u;
	}

	ProjectileReflectionService::Registration&
	ProjectileReflectionService::Registration::operator=(Registration&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			mDefenderId = other.mDefenderId;
			mGeneration = other.mGeneration;
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

		auto& entries = ReflectionEntries();
		const auto iterator = entries.find(mDefenderId);
		// The generation check keeps an older token from erasing a newer registration.
		if (iterator != entries.end() && iterator->second.generation == mGeneration)
		{
			entries.erase(iterator);
		}
		mDefenderId = 0u;
		mGeneration = 0u;
	}

	ProjectileReflectionService::Registration ProjectileReflectionService::RegisterReceiver(
		Actor& defender,
		ProjectileReflectionReceiver& receiver
	)
	{
		const unsigned int defenderId = defender.GetUniqueID();
		if (defenderId == 0u)
		{
			return Registration{};
		}

		const uint64_t generation = NextReflectionGeneration();
		// Explicit replace: the newest registration wins and an earlier token for the
		// same defender becomes inert, instead of silently keeping the old receiver.
		ReflectionEntries()[defenderId] = ReflectionEntry{
			MakeWeakActorHandle(defender),
			&receiver,
			generation
		};
		return Registration{ defenderId, generation };
	}

	bool ProjectileReflectionService::TryReflectProjectile(
		AbilityWorldActor& projectile,
		Actor& defender
	)
	{
		const auto iterator = ReflectionEntries().find(defender.GetUniqueID());
		if (iterator == ReflectionEntries().end())
		{
			return false;
		}

		const shared_ptr<Actor> liveDefender = iterator->second.defender.lock();
		return liveDefender.get() == &defender && iterator->second.receiver &&
			iterator->second.receiver->TryReflectIncomingProjectile(projectile, defender);
	}

	bool ProjectileReflectionService::TryReflectProjectileAlongPath(
		AbilityWorldActor& projectile,
		const sf::Vector2f& start,
		const sf::Vector2f& end
	)
	{
		auto& entries = ReflectionEntries();
		for (auto iterator = entries.begin(); iterator != entries.end();)
		{
			const shared_ptr<Actor> defender = iterator->second.defender.lock();
			ProjectileReflectionReceiver* receiver = iterator->second.receiver;
			if (!defender || !receiver || defender->GetIsPendingDestroy())
			{
				// Dead entries are pruned here, so a long run cannot accumulate stale
				// state from destroyed actors or retired worlds.
				iterator = entries.erase(iterator);
				continue;
			}

			// Advance before calling out: the receiver may end its own ability and
			// invalidate this iterator.
			++iterator;

			if (defender->GetWorld() != projectile.GetWorld())
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
