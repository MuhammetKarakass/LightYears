#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "AbilitySystemComponent.h"
#include "gameplay/tags/GameplayTags.h"

namespace ly::ability
{
	void ApplyFocusActionLocks(sas::AbilitySystemComponent& abilitySystem)
	{
		// Do not grant ExternalMovement here. Focus prevents player-issued
		// translation only; pulls, pushes and other world movement must still be
		// able to affect a concentrating ship.
		abilitySystem.AddOwnedTag(
			GameplayTags::State::ActionLock::AbilityActivation
		);
		abilitySystem.AddOwnedTag(
			GameplayTags::State::ActionLock::PrimaryWeaponFire
		);
		abilitySystem.AddOwnedTag(
			GameplayTags::State::ActionLock::MovementInput
		);
	}

	void RemoveFocusActionLocks(sas::AbilitySystemComponent& abilitySystem)
	{
		abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::AbilityActivation
		);
		abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::PrimaryWeaponFire
		);
		abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::MovementInput
		);
	}
}
