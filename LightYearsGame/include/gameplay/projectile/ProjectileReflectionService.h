#pragma once

#include "gameplay/projectile/ProjectileReflectionSurface.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace ly
{
	class Actor;
	class AbilityWorldActor;

	// Active counter abilities implement this policy. The projectile system only
	// asks whether a defender has a registered policy; it never knows a concrete
	// ability family such as Return Protocol.
	class ProjectileReflectionReceiver
	{
	public:
		virtual ~ProjectileReflectionReceiver() = default;
		virtual bool TryReflectIncomingProjectile(
			AbilityWorldActor& projectile,
			Actor& defender
		) = 0;
	};

	class ProjectileReflectionService
	{
	public:
		// RAII handle for one active reflection registration. The holder keeps the
		// registration alive and its destructor removes the entry, so correctness does
		// not depend on the owning ability's End() running. A behavior that is destroyed
		// without End() (for example its ship dies while the ability is active) still
		// unregisters, and the registry can never call into a dead receiver.
		class Registration
		{
		public:
			Registration() = default;
			~Registration();

			Registration(Registration&& other) noexcept;
			Registration& operator=(Registration&& other) noexcept;
			Registration(const Registration&) = delete;
			Registration& operator=(const Registration&) = delete;

			bool IsValid() const { return mDefenderId != 0u; }
			void Reset();

		private:
			friend class ProjectileReflectionService;
			Registration(unsigned int defenderId, uint64_t generation);

			unsigned int mDefenderId = 0u;
			// A newer registration for the same defender bumps the generation, so an
			// older token can never erase the newer entry.
			uint64_t mGeneration = 0u;
		};

		// Replaces any existing registration for the defender and returns its handle.
		// An invalid handle means the receiver could not be registered.
		static Registration RegisterReceiver(
			Actor& defender,
			ProjectileReflectionReceiver& receiver
		);
		static bool TryReflectProjectile(
			AbilityWorldActor& projectile,
			Actor& defender
		);
		// Non-physics delivery projectiles use this swept query while travelling.
		// It extends the same active receiver window to target-point deliveries.
		static bool TryReflectProjectileAlongPath(
			AbilityWorldActor& projectile,
			const sf::Vector2f& start,
			const sf::Vector2f& end
		);

		// Reflection from world geometry is intentionally independent from an
		// active defender receiver. The caller supplies the real swept-contact
		// normal so the outgoing angle obeys physical reflection.
		static bool TryReflectFromSurface(
			Actor& projectile,
			Actor& surface,
			const ProjectileReflectionSurfaceHit& hit
		);

		// Physics overlap callbacks do not carry the swept contact normal. Derive
		// the contacted face from the oriented surface and incoming trajectory so
		// slow projectiles and fast swept projectiles share the same reflection
		// policy.
		static bool TryReflectFromSurfaceOverlap(
			Actor& projectile,
			Actor& surface
		);
	};
}
