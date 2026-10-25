#pragma once

#include "gameplay/weapon/PrimaryWeaponMountProfile.h"

#include <optional>
#include <memory>

namespace vega
{
	class Actor;

	// Runtime behaviour of the Epic Primary mounts (Turret Mount, Broadside Mount). The rule struct ModulePrimaryMountRule lives
	// in ModuleDefinition.h because ModuleRulePackage stores it by value.
	namespace EpicPrimaryMounts
	{
		// A Broadside side volley starts this far from the ship centre, toward the side it fires from.
		inline constexpr float kBroadsideSpawnOffset = 24.f;

		// Turret Mount: the rotation (Actor::GetActorRotation convention) that points the forward axis at the chosen enemy in range.
		// Empty when the mount is not a Turret, when Broadside is also set, or when no valid enemy is in range.
		std::shared_ptr<Actor> FindTurretTarget(Actor& owner, const PrimaryWeaponMountProfile& mount, float range, unsigned int targetOrdinal = 0);
		std::optional<float> FindTurretRotation(Actor& owner, const PrimaryWeaponMountProfile& mount, float range, unsigned int targetOrdinal = 0);

		// Current over maximum health of an actor; 1 when it has no health component.
		float GetHealthFraction(const Actor& actor);
	}
}
