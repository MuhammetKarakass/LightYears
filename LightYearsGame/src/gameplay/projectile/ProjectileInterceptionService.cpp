#include "gameplay/projectile/ProjectileInterceptionService.h"

#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileInterceptionBoundary.h"

namespace ly
{
	bool ProjectileInterceptionService::TryInterceptProjectile(
		AbilityWorldActor& projectile,
		const sf::Vector2f& previousLocation
	)
	{
		World* world = projectile.GetWorld();
		if (!world || projectile.GetIsPendingDestroy())
		{
			return false;
		}

		// Circuit paths are limited by content to 1600 units. This conservative
		// broadphase finds every currently relevant world boundary without making
		// the projectile runtime know individual ability actor classes.
		constexpr float SearchRadius = 1800.f;
		const sf::FloatRect searchBounds{
			previousLocation - sf::Vector2f{ SearchRadius, SearchRadius },
			{ SearchRadius * 2.f, SearchRadius * 2.f }
		};
		const List<weak_ptr<Actor>> candidates = world->GetActorsInBounds(searchBounds);
		for (const weak_ptr<Actor>& candidateWeak : candidates)
		{
			const shared_ptr<Actor> candidate = candidateWeak.lock();
			if (!candidate || candidate.get() == &projectile ||
				candidate->GetWorld() != world || candidate->GetIsPendingDestroy())
			{
				continue;
			}

			auto* boundary = candidate
				? dynamic_cast<ProjectileInterceptionBoundary*>(candidate.get())
				: nullptr;
			if (boundary && !projectile.GetIsPendingDestroy() &&
				boundary->TryInterceptProjectile(projectile, previousLocation))
			{
				return true;
			}
			// A boundary implementation may consume the projectile as part of its
			// own response. Treat that as a successful interception and never allow
			// another boundary to process the same pending-destroy projectile.
			if (projectile.GetIsPendingDestroy())
			{
				return true;
			}
		}
		return false;
	}
}
