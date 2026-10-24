#include "gameplay/module/rules/EpicPrimaryRules.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <cmath>

namespace vega::EpicPrimaryMounts
{
	float GetHealthFraction(const Actor& actor)
	{
		const HealthComponent* health = actor.FindComponent<HealthComponent>();
		return health && health->GetMaxHealth() > 0.f ? health->GetHealth() / health->GetMaxHealth() : 1.f;
	}

	std::optional<float> FindTurretRotation(Actor& owner, const PrimaryWeaponMountProfile& mount, float range)
	{
		if (!mount.turret || mount.broadside) return std::nullopt;
		World* world = owner.GetWorld();
		if (!world) return std::nullopt;

		const sf::Vector2f origin = owner.GetActorLocation();
		shared_ptr<Actor> chosen;
		float chosenHealth = 0.f;
		float chosenDistance = 0.f;
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(*world, owner, origin, range, false))
		{
			if (!candidate) continue;
			const sf::Vector2f offset = candidate->GetActorLocation() - origin;
			const float distance = offset.x * offset.x + offset.y * offset.y;
			// Nearest first; Execution Priority compares the health fraction before distance.
			const float health = mount.executionPriority ? GetHealthFraction(*candidate) : 0.f;
			const bool better = !chosen || health < chosenHealth ||
				(health == chosenHealth && (distance < chosenDistance || (distance == chosenDistance && candidate->GetUniqueID() < chosen->GetUniqueID())));
			if (!better) continue;
			chosen = candidate;
			chosenHealth = health;
			chosenDistance = distance;
		}
		if (!chosen) return std::nullopt;

		const sf::Vector2f toward = chosen->GetActorLocation() - origin;
		// Projectile forward is RotationToVector(rotation - 90), so the rotation that faces the target adds 90 to its bearing.
		return RadiansToDegrees(std::atan2(toward.y, toward.x)) + 90.f;
	}
}
