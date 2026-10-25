#include "gameplay/module/rules/EpicPrimaryRules.h"

#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/targeting/CombatantTargetQuery.h"

#include <cmath>
#include <algorithm>

namespace vega::EpicPrimaryMounts
{
	float GetHealthFraction(const Actor& actor)
	{
		const HealthComponent* health = actor.FindComponent<HealthComponent>();
		return health && health->GetMaxHealth() > 0.f ? health->GetHealth() / health->GetMaxHealth() : 1.f;
	}

	std::shared_ptr<Actor> FindTurretTarget(Actor& owner, const PrimaryWeaponMountProfile& mount, float range, unsigned int targetOrdinal)
	{
		if (!mount.turret || mount.broadside) return nullptr;
		World* world = owner.GetWorld();
		if (!world) return nullptr;

		const sf::Vector2f origin = owner.GetActorLocation();
		auto targets = targeting::FindOpposingCombatants(*world, owner, origin, range, false);
		std::sort(targets.begin(), targets.end(), [&](const auto& left, const auto& right)
		{
			const float leftHealth = mount.executionPriority ? GetHealthFraction(*left) : 0.f;
			const float rightHealth = mount.executionPriority ? GetHealthFraction(*right) : 0.f;
			if (leftHealth != rightHealth) return leftHealth < rightHealth;
			const auto l = left->GetActorLocation() - origin, r = right->GetActorLocation() - origin;
			const float ld = l.x * l.x + l.y * l.y, rd = r.x * r.x + r.y * r.y;
			return ld == rd ? left->GetUniqueID() < right->GetUniqueID() : ld < rd;
		});
		const auto chosen = targetOrdinal < targets.size() ? targets[targetOrdinal] : nullptr;
		return chosen;
	}

	std::optional<float> FindTurretRotation(Actor& owner, const PrimaryWeaponMountProfile& mount, float range, unsigned int targetOrdinal)
	{
		const auto chosen = FindTurretTarget(owner, mount, range, targetOrdinal);
		if (!chosen) return std::nullopt;
		const auto origin = owner.GetActorLocation();
		const sf::Vector2f toward = chosen->GetActorLocation() - origin;
		// Projectile forward is RotationToVector(rotation - 90), so the rotation that faces the target adds 90 to its bearing.
		return RadiansToDegrees(std::atan2(toward.y, toward.x)) + 90.f;
	}
}
