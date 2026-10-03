#include "effects/GameplayEffectLifecycleOrchestrator.h"

#include <algorithm>
#include <cmath>

namespace sas
{
	bool GameplayEffectLifecycleOrchestrator::IsApplicableSpec(const GameplayEffectSpec& spec)
	{
		const GameplayEffectDefinition& definition = spec.definition;
		const bool durationValid = definition.durationPolicy != GameplayEffectDurationPolicy::Duration ||
			(std::isfinite(spec.duration) && spec.duration > 0.f);
		const bool decayValid = definition.stackLifetimePolicy != GameplayEffectStackLifetimePolicy::DecayAfterDuration ||
			(std::isfinite(definition.stackDecayInterval) && definition.stackDecayInterval > 0.f);
		return spec.maxStacks >= 1 && durationValid && decayValid;
	}

	bool GameplayEffectLifecycleOrchestrator::IsDecayAfterDuration(const GameplayEffectDefinition& definition)
	{
		return definition.durationPolicy == GameplayEffectDurationPolicy::Duration &&
			definition.stackLifetimePolicy == GameplayEffectStackLifetimePolicy::DecayAfterDuration;
	}

	bool GameplayEffectLifecycleOrchestrator::GrantsImmunity(
		const std::string& grantedCategory,
		const std::string& blockedCategory
	)
	{
		return grantedCategory == blockedCategory ||
			(!grantedCategory.empty() && blockedCategory.rfind(grantedCategory + ".", 0) == 0);
	}

	void GameplayEffectLifecycleOrchestrator::RefreshCappedSpec(
		GameplayEffectSpec& activeSpec,
		const GameplayEffectSpec& incomingSpec
	)
	{
		activeSpec.duration = incomingSpec.duration;
		activeSpec.maxStacks = incomingSpec.maxStacks;
		activeSpec.sourceAbilityUpgradeIds = incomingSpec.sourceAbilityUpgradeIds;
	}

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
			const int stackCountBefore = state.stackCount;
			const bool stackChangedOrExpired = state.TickStackDecay(deltaTime);
			return GameplayEffectDurationTickResult{
				stackChangedOrExpired || deltaTime > 0.f,
				state.stackCount == 0,
				state.stackCount != stackCountBefore
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
