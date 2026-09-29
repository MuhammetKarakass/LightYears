#pragma once

namespace sas
{
	class AbilitySystemComponent;
}

namespace ly::ability
{
	// A focus window consumes player commands while preserving world-driven
	// movement and rotation. It is intentionally a fixed bundle: abilities may
	// choose whether they focus, but may not silently redefine what focus means.
	// Some focus abilities (ShieldHarvest) only lock ability activation and the
	// primary weapon while the ship stays free to move. Apply/Remove must be
	// called with the same movement policy.
	void ApplyFocusActionLocks(
		sas::AbilitySystemComponent& abilitySystem,
		bool lockMovementInput = true
	);
	void RemoveFocusActionLocks(
		sas::AbilitySystemComponent& abilitySystem,
		bool lockMovementInput = true
	);
}
