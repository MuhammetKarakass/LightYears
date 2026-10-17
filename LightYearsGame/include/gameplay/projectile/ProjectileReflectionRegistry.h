#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace vega
{
	class AbilityWorldActor;
	class ProjectileReflectionReceiver;
	class ProjectileReflectionService;

	// Registrations lock state directly; cleanup never dereferences a removed service.
	class ProjectileReflectionRegistryState
	{
	public:
		void UnregisterReceiver(unsigned int defenderId, uint64_t generation);

	private:
		friend class ProjectileReflectionRegistry;
		struct Entry
		{
			weak_ptr<Actor> defender;
			ProjectileReflectionReceiver* receiver = nullptr;
			uint64_t generation = 0u;
		};
		std::unordered_map<unsigned int, Entry> entries;
	};

	class ProjectileReflectionRegistry final
	{
	public:
		explicit ProjectileReflectionRegistry(World* world);
		ProjectileReflectionRegistry(const ProjectileReflectionRegistry&) = delete;
		ProjectileReflectionRegistry& operator=(const ProjectileReflectionRegistry&) = delete;
		static ProjectileReflectionRegistry* GetOrCreate(World& world);

		uint64_t RegisterReceiver(Actor& defender, ProjectileReflectionReceiver& receiver);
		void UnregisterReceiver(unsigned int defenderId, uint64_t generation);
		bool TryReflectProjectile(AbilityWorldActor& projectile, Actor& defender);
		bool TryReflectProjectileAlongPath(AbilityWorldActor& projectile, const sf::Vector2f& start, const sf::Vector2f& end);

	private:
		friend class ProjectileReflectionService;
		World* const mWorld;
		shared_ptr<ProjectileReflectionRegistryState> mState = std::make_shared<ProjectileReflectionRegistryState>();
		uint64_t mNextGeneration = 0u;
	};
}
