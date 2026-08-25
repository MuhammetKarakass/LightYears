#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class Actor;

	// A physical surface supplies the contact geometry and policy, while the
	// reflection service owns the shared vector math and short same-surface
	// lock. This lets walls, mirrors, and future angled shields share a single
	// projectile contract without knowing projectile families.
	struct ProjectileReflectionSurfaceHit
	{
		sf::Vector2f impactLocation{ 0.f, 0.f };
		sf::Vector2f surfaceNormal{ 0.f, -1.f };
	};

	struct ProjectileReflectionSurfaceResponse
	{
		Actor* newOwner = nullptr;
		float damageMultiplier = 1.f;
		float sameSurfaceLockDuration = 0.12f;
		bool allowSameOwnerReflection = true;
	};

	class ProjectileReflectionSurface
	{
	public:
		virtual ~ProjectileReflectionSurface() = default;

		virtual bool BuildProjectileReflectionResponse(
			const Actor& incomingProjectile,
			ProjectileReflectionSurfaceResponse& outResponse
		) const = 0;
	};
}
