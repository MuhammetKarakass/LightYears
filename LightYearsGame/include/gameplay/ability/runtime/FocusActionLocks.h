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
	void ApplyFocusActionLocks(sas::AbilitySystemComponent& abilitySystem);
	void RemoveFocusActionLocks(sas::AbilitySystemComponent& abilitySystem);
}
