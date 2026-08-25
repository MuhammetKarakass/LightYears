#pragma once

#include <SFML/System/Vector2.hpp>
#include "gameplay/projectile/ProjectileReflectionSurface.h"

namespace ly
{
	class Actor;

	// A travelling projectile family opts into in-place reflection through this
	// capability. Unlike Relay conversion, reflection keeps the same actor and
	// its family-specific runtime state (remaining range, payload, pierce, etc.).
	struct ProjectileReflectionRequest
	{
		Actor& newOwner;
		sf::Vector2f returnDirection{ 0.f, -1.f };
		float damageMultiplier = 1.f;
		// A physical wall may bounce a projectile fired by the same owner. Active
		// defender abilities leave this false, preserving their anti-self-return
		// policy while surfaces use their explicit geometry contract.
		bool allowSameOwnerReflection = false;
	};

	class ProjectileReflectionParticipant
	{
	public:
		virtual ~ProjectileReflectionParticipant() = default;

		virtual bool CanBeReflected() const = 0;
		virtual bool TryReflectProjectile(
			const ProjectileReflectionRequest& request
		) = 0;
	};

}
