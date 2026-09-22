#pragma once

namespace sas
{
	enum class GameplayEffectDurationPolicy
	{
		Instant,
		Duration,
		Infinite
	};

	enum class GameplayEffectStackingPolicy
	{
		None,
		RefreshDuration,
		Stack
	};

	enum class GameplayEffectStackLifetimePolicy
	{
		None,
		DecayAfterDuration
	};

	enum class GameplayEffectDisposition
	{
		Beneficial,
		Harmful,
		Neutral
	};
}
