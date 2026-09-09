#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
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
