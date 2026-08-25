#pragma once

#include "gameplay/projectile/ProjectileReflectionSurface.h"

#include <SFML/System/Vector2.hpp>

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
		static bool RegisterReceiver(
			Actor& defender,
			ProjectileReflectionReceiver& receiver
		);
		static void UnregisterReceiver(
			Actor& defender,
			const ProjectileReflectionReceiver& receiver
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
