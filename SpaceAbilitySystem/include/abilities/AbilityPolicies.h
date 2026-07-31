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
