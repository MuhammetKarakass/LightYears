#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "effects/GameplayEffectRuntimeState.h"
#include "effects/GameplayEffectSpec.h"

#include <string>

namespace sas
{
	enum class GameplayEffectApplicationKind
	{
		Instant,
		CreateActive,
		RefreshActive,
		AddStack,
		RefreshStackDuration
	};

	struct GameplayEffectDurationTickResult
	{
		bool changed = false;
		bool expired = false;
		bool stackChanged = false;
	};

	class GameplayEffectLifecycleOrchestrator
	{
	public:
		static bool IsApplicableSpec(const GameplayEffectSpec& spec);
		static bool IsDecayAfterDuration(const GameplayEffectDefinition& definition);
		// Immunity categories are a dot-separated hierarchy: a provider of "Control"
		// blocks "Control.Stun", while "Movement.Slow" stays narrow.
		static bool GrantsImmunity(const std::string& grantedCategory, const std::string& blockedCategory);
		// Refreshes the duration-facing spec fields when a capped stack is hit again.
		static void RefreshCappedSpec(GameplayEffectSpec& activeSpec, const GameplayEffectSpec& incomingSpec);
		static bool MatchesStackingTarget(
			const GameplayEffectDefinition& definition,
			const std::string& activeEffectId,
			const void* activeSourceScope,
			const void* incomingSourceScope
		);
		static GameplayEffectApplicationKind ResolveApplication(
			const GameplayEffectDefinition& definition,
			bool hasStackingTarget,
			int currentStackCount,
			int resolvedMaxStacks
		);
		static bool ApplyStackingState(
			GameplayEffectApplicationKind applicationKind,
			GameplayEffectRuntimeState& state,
			float duration,
			float stackDecayInterval,
			int resolvedMaxStacks
		);
		static GameplayEffectDurationTickResult TickDuration(
			GameplayEffectDurationPolicy durationPolicy,
			GameplayEffectStackLifetimePolicy stackLifetimePolicy,
			GameplayEffectRuntimeState& state,
			float deltaTime
		);
		static float GetTickSliceDuration(
			GameplayEffectDurationPolicy durationPolicy,
			GameplayEffectStackLifetimePolicy stackLifetimePolicy,
			const GameplayEffectRuntimeState& state,
			float deltaTime
		);
		static bool CanRefreshDuration(GameplayEffectDurationPolicy durationPolicy);
	};
}
