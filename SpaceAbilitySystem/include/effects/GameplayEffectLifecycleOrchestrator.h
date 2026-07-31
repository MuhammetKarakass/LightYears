#pragma once

#include "effects/GameplayEffectDefinition.h"
#include "effects/GameplayEffectRuntimeState.h"

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
	};

	class GameplayEffectLifecycleOrchestrator
	{
	public:
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
			int resolvedMaxStacks
		);
		static GameplayEffectDurationTickResult TickDuration(
			GameplayEffectDurationPolicy durationPolicy,
			GameplayEffectRuntimeState& state,
			float deltaTime
		);
		static bool CanRefreshDuration(GameplayEffectDurationPolicy durationPolicy);
	};
}
