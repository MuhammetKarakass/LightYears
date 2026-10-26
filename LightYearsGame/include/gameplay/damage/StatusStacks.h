#pragma once

#include "framework/Core.h"
#include "framework/GameplayTag.h"

namespace sas
{
	class AbilitySystemComponent;
}

namespace vega
{
	class Actor;
	struct DamageContext;

	// One stacking damage status (Ignite, Cryo slow, Electric, Kinetic) on a target at one moment.
	struct DamageStatusStackState
	{
		GameplayTag status;
		int stacks = 0;
		int maxStacks = 0;
		float remainingSeconds = 0.f;
	};

	// The stacking damage statuses in their fixed order Thermal(Ignite) / Cryo / Electric / Kinetic. Frozen, freeze
	// cooldown and status-like control effects are not part of it.
	const List<GameplayTag>& GetStackingDamageStatuses();
	// Present stacking damage statuses of the target, in the fixed order.
	List<DamageStatusStackState> CaptureDamageStatusStacks(const sas::AbilitySystemComponent& target);
	const DamageStatusStackState* FindDamageStatusStack(const List<DamageStatusStackState>& states, const GameplayTag& status);
	// Removes every stack and the remaining time of one stacking damage status (an Ignite's pending DoT is cancelled, not
	// delivered). Returns false when the target did not carry it.
	bool ConsumeDamageStatus(sas::AbilitySystemComponent& target, const GameplayTag& status);

	// Broadcast on the SOURCE combatant's CombatRuntime after a hit's status application added at least one new stack.
	// A refresh at the stack cap is not a new stack; a stack added and then spent by the same hit (Frost Lock, Kinetic
	// Breaker, Ember) still is. before holds the target's statuses as they were before this hit's status application.
	struct StatusStacksAddedEvent
	{
		Actor* target = nullptr;
		List<GameplayTag> addedStatuses;
		List<DamageStatusStackState> before;
		const DamageContext* context = nullptr;
	};
	// Statuses the hit added a new stack of, from the statuses it applied and the stack states around the application.
	List<GameplayTag> ResolveAddedDamageStatuses(const List<GameplayTag>& appliedStatuses, const List<DamageStatusStackState>& before, const List<DamageStatusStackState>& after);
}
