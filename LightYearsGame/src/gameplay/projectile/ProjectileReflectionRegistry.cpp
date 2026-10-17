#include "gameplay/projectile/ProjectileReflectionRegistry.h"

#include "framework/MathUtility.h"
#include "framework/World.h"
#include "framework/WorldServices.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionService.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		float DistanceSquaredToSegment(const sf::Vector2f& point, const sf::Vector2f& start, const sf::Vector2f& end)
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
			const float fraction = std::clamp((pointOffset.x * segment.x + pointOffset.y * segment.y) / segmentLengthSquared, 0.f, 1.f);
			const sf::Vector2f offset = point - (start + segment * fraction);
			return offset.x * offset.x + offset.y * offset.y;
		}
	}

	ProjectileReflectionRegistry::ProjectileReflectionRegistry(World* world)
		: mWorld(world)
	{
	}

	ProjectileReflectionRegistry* ProjectileReflectionRegistry::GetOrCreate(World& world)
	{
		try { return &world.Services().GetOrCreate<ProjectileReflectionRegistry>(&world); }
		catch (const std::logic_error&)
		{
			// Services close before actor teardown; destructor-time registration returns empty.
			return nullptr;
		}
	}

	uint64_t ProjectileReflectionRegistry::RegisterReceiver(Actor& defender, ProjectileReflectionReceiver& receiver)
	{
		const unsigned int defenderId = defender.GetUniqueID();
		if (defenderId == 0u || defender.GetIsPendingDestroy() ||
			!mWorld || defender.GetWorld() != mWorld)
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
		mState->entries[defenderId] = ProjectileReflectionRegistryState::Entry{ liveDefender, &receiver, mNextGeneration };
		return mNextGeneration;
	}

	void ProjectileReflectionRegistryState::UnregisterReceiver(unsigned int defenderId, uint64_t generation)
	{
		const auto entry = entries.find(defenderId);
		if (entry != entries.end() && entry->second.generation == generation)
		{
			entries.erase(entry);
		}
	}

	void ProjectileReflectionRegistry::UnregisterReceiver(unsigned int defenderId, uint64_t generation)
	{
		mState->UnregisterReceiver(defenderId, generation);
	}

	bool ProjectileReflectionRegistry::TryReflectProjectile(AbilityWorldActor& projectile, Actor& defender)
	{
		if (!mWorld || projectile.GetWorld() != mWorld ||
			defender.GetWorld() != mWorld || projectile.GetIsPendingDestroy() || defender.GetIsPendingDestroy())
		{
			return false;
		}
		const auto entry = mState->entries.find(defender.GetUniqueID());
		if (entry == mState->entries.end())
		{
			return false;
		}

		const shared_ptr<Actor> liveDefender = entry->second.defender.lock();
		return liveDefender.get() == &defender && entry->second.receiver &&
			entry->second.receiver->TryReflectIncomingProjectile(projectile, defender);
	}

	bool ProjectileReflectionRegistry::TryReflectProjectileAlongPath(AbilityWorldActor& projectile, const sf::Vector2f& start, const sf::Vector2f& end)
	{
		if (!mWorld || projectile.GetWorld() != mWorld ||
			projectile.GetIsPendingDestroy()) return false;
		for (auto entry = mState->entries.begin(); entry != mState->entries.end();)
		{
			const shared_ptr<Actor> defender = entry->second.defender.lock();
			ProjectileReflectionReceiver* receiver = entry->second.receiver;
			if (!defender || !receiver || defender->GetIsPendingDestroy())
			{
				entry = mState->entries.erase(entry);
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
