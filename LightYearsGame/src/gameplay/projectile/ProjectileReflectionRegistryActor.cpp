#include "gameplay/projectile/ProjectileReflectionRegistryActor.h"

#include "framework/MathUtility.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionService.h"

#include <algorithm>

namespace ly
{
	namespace
	{
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
	}

	ProjectileReflectionRegistryActor::ProjectileReflectionRegistryActor(World* world)
		: Actor(world)
	{
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	uint64_t ProjectileReflectionRegistryActor::RegisterReceiver(
		Actor& defender,
		ProjectileReflectionReceiver& receiver
	)
	{
		const unsigned int defenderId = defender.GetUniqueID();
		if (defenderId == 0u || defender.GetIsPendingDestroy() ||
			!GetWorld() || defender.GetWorld() != GetWorld())
		{
			return 0u;
		}

		const shared_ptr<Object> object = defender.GetWeakPtr().lock();
		const shared_ptr<Actor> liveDefender = object
			? std::dynamic_pointer_cast<Actor>(object)
			: shared_ptr<Actor>{};
		if (!liveDefender || liveDefender.get() != &defender)
		{
			return 0u;
		}

		++mNextGeneration;
		if (mNextGeneration == 0u)
		{
			++mNextGeneration;
		}
		mEntries[defenderId] = Entry{ liveDefender, &receiver, mNextGeneration };
		return mNextGeneration;
	}

	void ProjectileReflectionRegistryActor::UnregisterReceiver(
		unsigned int defenderId,
		uint64_t generation
	)
	{
		const auto entry = mEntries.find(defenderId);
		if (entry != mEntries.end() && entry->second.generation == generation)
		{
			mEntries.erase(entry);
		}
	}

	bool ProjectileReflectionRegistryActor::TryReflectProjectile(
		AbilityWorldActor& projectile,
		Actor& defender
	)
	{
		if (GetIsPendingDestroy() || !GetWorld() || projectile.GetWorld() != GetWorld() ||
			defender.GetWorld() != GetWorld() || projectile.GetIsPendingDestroy() || defender.GetIsPendingDestroy())
		{
			return false;
		}
		const auto entry = mEntries.find(defender.GetUniqueID());
		if (entry == mEntries.end())
		{
			return false;
		}

		const shared_ptr<Actor> liveDefender = entry->second.defender.lock();
		return liveDefender.get() == &defender && entry->second.receiver &&
			entry->second.receiver->TryReflectIncomingProjectile(projectile, defender);
	}

	bool ProjectileReflectionRegistryActor::TryReflectProjectileAlongPath(
		AbilityWorldActor& projectile,
		const sf::Vector2f& start,
		const sf::Vector2f& end
	)
	{
		if (GetIsPendingDestroy() || !GetWorld() || projectile.GetWorld() != GetWorld() ||
			projectile.GetIsPendingDestroy()) return false;
		for (auto entry = mEntries.begin(); entry != mEntries.end();)
		{
			const shared_ptr<Actor> defender = entry->second.defender.lock();
			ProjectileReflectionReceiver* receiver = entry->second.receiver;
			if (!defender || !receiver || defender->GetIsPendingDestroy())
			{
				entry = mEntries.erase(entry);
				continue;
			}

			++entry;
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
}
