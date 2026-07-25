#pragma once

#include "gameplay/weapon/PrimaryWeaponHandler.h"

namespace ly
{
	class ProjectileImpactBehavior;

	// Shared arcade projectile momentum. These are intentionally global weapon rules,
	// not per-ship attributes or progression values.
	namespace ProjectileMotion
	{
		inline constexpr float ForwardVelocityInheritance = 0.75f;
		inline constexpr float BackwardVelocityInheritance = 0.f;
		inline constexpr float LateralVelocityInheritance = 0.05f;

		sf::Vector2f ResolveCarrierVelocity(
			const sf::Vector2f& ownerVelocity,
			const sf::Vector2f& fireDirection
		);
	}

	namespace PrimaryWeaponProjectileSpawner
	{
		void FireSet(
			const PrimaryWeaponExecutionContext& context,
			int projectileCount,
			float spreadAngle,
			const shared_ptr<ProjectileImpactBehavior>& impactBehavior = {}
		);
	}
}
