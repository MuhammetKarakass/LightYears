#pragma once

#include "framework/Actor.h"

#include <cstdint>
#include <unordered_map>

namespace ly
{
	class AbilityWorldActor;
	class ProjectileReflectionReceiver;

	class ProjectileReflectionRegistryActor final : public Actor
	{
	public:
		explicit ProjectileReflectionRegistryActor(World* world);

		uint64_t RegisterReceiver(Actor& defender, ProjectileReflectionReceiver& receiver);
		void UnregisterReceiver(unsigned int defenderId, uint64_t generation);
		bool TryReflectProjectile(AbilityWorldActor& projectile, Actor& defender);
		bool TryReflectProjectileAlongPath(
			AbilityWorldActor& projectile,
			const sf::Vector2f& start,
			const sf::Vector2f& end
		);

	private:
		struct Entry
		{
			weak_ptr<Actor> defender;
			ProjectileReflectionReceiver* receiver = nullptr;
			uint64_t generation = 0u;
		};

		std::unordered_map<unsigned int, Entry> mEntries;
		uint64_t mNextGeneration = 0u;
	};
}
