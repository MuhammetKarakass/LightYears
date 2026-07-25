#pragma once

namespace ly
{
	class Actor;

	// Optional policy shared by projectiles that coordinate or replace direct-hit damage.
	class ProjectileImpactBehavior
	{
	public:
		virtual ~ProjectileImpactBehavior() = default;
		virtual void OnProjectileSpawned() = 0;
		virtual bool HandleImpact(Actor& target) = 0;
		virtual void OnProjectileFinished() = 0;
	};
}
