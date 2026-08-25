#pragma once

#include "framework/Actor.h"

namespace ly::DirectionalBarrierEffectBehavior
{
	bool RegisterDirectionalBarrierEffectBehavior();

	// Shared physical projectiles call this before the physics step. The
	// previous/current segment lets fast projectiles hit the visible boundary
	// instead of waiting for the ship body collision.
	bool TryInterceptProjectile(
		Actor& projectile,
		const sf::Vector2f& previousLocation
	);
}
