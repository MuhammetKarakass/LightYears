#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class AbilityWorldActor;

	// Implemented by world actors that can consume a travelling projectile. The
	// boundary owns its own allegiance and durability rules; the common service
	// only discovers boundaries, so projectile families never depend on a named
	// defensive ability.
	class ProjectileInterceptionBoundary
	{
	public:
		virtual ~ProjectileInterceptionBoundary() = default;
		virtual bool TryInterceptProjectile(
			AbilityWorldActor& projectile,
			const sf::Vector2f& previousLocation
		) = 0;
	};
}
