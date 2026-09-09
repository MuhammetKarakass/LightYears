#pragma once

namespace sas
{
	// Toggle abilities share one gameplay rule: the player must commit to the
	// active state for at least one second before a second press may deactivate
	// it. System-driven cancellation (interruption, stun, owner destruction) is
	// intentionally separate and is not filtered by this input rule.
	inline constexpr float ToggleMinimumActiveDurationSeconds = 1.f;

	enum class AbilitySlot
	{
		None,
		PrimaryFire,
		Ability1,
		Ability2,
		Ability3,
		Ability4
	};

	// These are the four swappable runtime loadout bindings. PrimaryFire is
	// reserved for a ship's weapon and None is reserved for passive abilities.
	constexpr bool IsLoadoutAbilitySlot(AbilitySlot slot)
	{
		return slot == AbilitySlot::Ability1 ||
			slot == AbilitySlot::Ability2 ||
			slot == AbilitySlot::Ability3 ||
			slot == AbilitySlot::Ability4;
	}

	enum class AbilityActivationPolicy
	{
		OnPressed,
		WhileHeld,
		Toggle,
		Passive,
		GameplayEvent
	};

	enum class AbilityLifetimePolicy
	{
		Instant,
		Duration,
		WhileInputHeld,
		UntilCancelled
	};

	// Most duration abilities begin recovery when they end. A few deliberate
	// non-channel mechanics (for example a drawing window) must begin recovery
	// on input while remaining active; keeping that distinction in the shared
	// policy layer avoids family-specific cooldown workarounds.
	enum class AbilityCooldownStartPolicy
	{
		OnAbilityEnd,
		OnActivation
	};

	// Lifecycle consumers use this origin to distinguish a normal player cast
	// from a system-generated invocation. Keeping the origin in the shared
	// policy layer prevents history, checkpoint and analytics systems from
	// inferring it from ability IDs.
	enum class AbilityActivationOrigin
	{
		NormalInput,
		EchoInvocation,
		TriggeredAction,
		System
	};

	enum class AbilityActionPhase
	{
		OnActivate,
		WhileActive,
		OnEnd
	};

	enum class AbilityEndReason
	{
		Completed,
		DurationExpired,
		InputReleased,
		Cancelled,
		Interrupted,
		OwnerDestroyed
	};

	enum class AbilityTargetPolicy
	{
		Self,
		OwnerForward,
		EventTarget,
		EventSource
	};

	enum class AbilitySpawnPolicy
	{
		AtOwner,
		OwnerForward,
		AtEventTarget,
		MouseWorld
	};

	enum class AbilityDirectionPolicy
	{
		OwnerForward,
		OwnerVelocity,
		MouseWorld
	};
}
