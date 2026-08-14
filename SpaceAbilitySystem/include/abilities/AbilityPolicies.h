#pragma once

namespace sas
{
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
