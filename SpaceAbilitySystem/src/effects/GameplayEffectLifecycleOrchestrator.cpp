#include "effects/GameplayEffectLifecycleOrchestrator.h"

#include <algorithm>

namespace sas
{
	bool GameplayEffectLifecycleOrchestrator::MatchesStackingTarget(
		const GameplayEffectDefinition& definition,
		const std::string& activeEffectId,
		const void* activeSourceScope,
		const void* incomingSourceScope
	)
	{
		return definition.stackingPolicy != GameplayEffectStackingPolicy::None &&
			activeEffectId == definition.effectId &&
			(!definition.sourceScopedApplication ||
				activeSourceScope == incomingSourceScope);
	}

	GameplayEffectApplicationKind
	GameplayEffectLifecycleOrchestrator::ResolveApplication(
		const GameplayEffectDefinition& definition,
		bool hasStackingTarget,
		int currentStackCount,
		int resolvedMaxStacks
	)
	{
		if (definition.durationPolicy == GameplayEffectDurationPolicy::Instant)
		{
			return GameplayEffectApplicationKind::Instant;
		}
		if (!hasStackingTarget ||
			definition.stackingPolicy == GameplayEffectStackingPolicy::None)
		{
			return GameplayEffectApplicationKind::CreateActive;
		}
		if (definition.stackingPolicy == GameplayEffectStackingPolicy::RefreshDuration)
		{
			return GameplayEffectApplicationKind::RefreshActive;
		}
		return currentStackCount < std::max(1, resolvedMaxStacks)
			? GameplayEffectApplicationKind::AddStack
			: GameplayEffectApplicationKind::RefreshStackDuration;
	}

	bool GameplayEffectLifecycleOrchestrator::ApplyStackingState(
		GameplayEffectApplicationKind applicationKind,
		GameplayEffectRuntimeState& state,
		float duration,
		float stackDecayInterval,
		int resolvedMaxStacks
	)
	{
		bool stackAdded = false;
		if (applicationKind == GameplayEffectApplicationKind::RefreshActive)
		{
			state.ResetStackCount();
		}
		else if (applicationKind == GameplayEffectApplicationKind::AddStack)
		{
			stackAdded = state.TryAddStack(resolvedMaxStacks);
		}
		state.RefreshDuration(duration, stackDecayInterval);
		return stackAdded;
	}

	GameplayEffectDurationTickResult
	GameplayEffectLifecycleOrchestrator::TickDuration(
		GameplayEffectDurationPolicy durationPolicy,
		GameplayEffectStackLifetimePolicy stackLifetimePolicy,
		GameplayEffectRuntimeState& state,
		float deltaTime
	)
	{
		if (durationPolicy != GameplayEffectDurationPolicy::Duration)
		{
			return {};
		}
		if (stackLifetimePolicy == GameplayEffectStackLifetimePolicy::DecayAfterDuration)
		{
			const bool stackChangedOrExpired = state.TickStackDecay(deltaTime);
			return GameplayEffectDurationTickResult{
				stackChangedOrExpired || deltaTime > 0.f,
				state.stackCount == 0
			};
		}
		return GameplayEffectDurationTickResult{ true, state.TickDuration(deltaTime) };
	}

	float GameplayEffectLifecycleOrchestrator::GetTickSliceDuration(
		GameplayEffectDurationPolicy durationPolicy,
		GameplayEffectStackLifetimePolicy stackLifetimePolicy,
		const GameplayEffectRuntimeState& state,
		float deltaTime
	)
	{
		if (durationPolicy != GameplayEffectDurationPolicy::Duration ||
			stackLifetimePolicy != GameplayEffectStackLifetimePolicy::DecayAfterDuration ||
			deltaTime <= 0.f)
		{
			return deltaTime;
		}
		return std::min(deltaTime, std::max(0.f, state.remainingDuration));
	}

	bool GameplayEffectLifecycleOrchestrator::CanRefreshDuration(
		GameplayEffectDurationPolicy durationPolicy
	)
	{
		return durationPolicy == GameplayEffectDurationPolicy::Duration;
	}
}
