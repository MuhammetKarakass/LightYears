#pragma once

#include <SFML/System/Vector2.hpp>

namespace vega
{
	class AbilityWorldActor;

	// Implemented by world actors that can consume a travelling projectile. The
	// boundary owns its own allegiance and durability rules; the common service
	// only discovers boundaries, so projectile families never depend on a named
	// defensive ability.
	class ProjectileInterceptionBoundary
	{
	public:
		virtual ~ProjectileInterceptionBoundary() { --sLiveBoundaries; }
		virtual bool TryInterceptProjectile(
			AbilityWorldActor& projectile,
			const sf::Vector2f& previousLocation
		) = 0;

		// Boundaries alive in any world. Every projectile asks the service every frame, so with none alive
		// (the usual case) the service skips its world scan entirely.
		static int GetLiveCount() { return sLiveBoundaries; }

	protected:
		ProjectileInterceptionBoundary() { ++sLiveBoundaries; }
		ProjectileInterceptionBoundary(const ProjectileInterceptionBoundary&) { ++sLiveBoundaries; }
		ProjectileInterceptionBoundary& operator=(const ProjectileInterceptionBoundary&) = default;

	private:
		inline static int sLiveBoundaries = 0;
	};
}
