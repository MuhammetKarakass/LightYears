#pragma once

#include <SFML/System/Vector2.hpp>

namespace vega
{
	class AbilityWorldActor;

	class ProjectileInterceptionService final
	{
	public:
		static bool TryInterceptProjectile(
			AbilityWorldActor& projectile,
			const sf::Vector2f& previousLocation
		);
	};
}
