#include "effects/GameplayEffectRuntimeState.h"

#include <algorithm>

namespace sas
{
	void GameplayEffectRuntimeState::Initialize(
		GameplayEffectHandle newHandle,
		float duration,
		const GameplayAttributeList& attributes
	)
	{
		handle = newHandle;
		RefreshDuration(duration);
		stackCount = 1;
		appliedModifierHandles.clear();
		ResetRuntimeAttributes(attributes);
	}

	void GameplayEffectRuntimeState::RefreshDuration(float duration)
	{
		remainingDuration = duration;
		totalDuration = duration;
	}

	bool GameplayEffectRuntimeState::TickDuration(float deltaTime)
	{
		remainingDuration -= deltaTime;
		return remainingDuration <= 0.f;
	}

	void GameplayEffectRuntimeState::ResetStackCount()
	{
		stackCount = 1;
	}

	bool GameplayEffectRuntimeState::TryAddStack(int maxStacks)
	{
		if (stackCount >= std::max(1, maxStacks))
		{
			return false;
		}
		++stackCount;
		return true;
	}

	void GameplayEffectRuntimeState::ResetRuntimeAttributes(
		const GameplayAttributeList& attributes
	)
	{
		runtimeAttributes = attributes;
		for (GameplayAttribute& attribute : runtimeAttributes)
		{
			attribute.currentValue = attribute.baseValue;
		}
	}

	GameplayEffectRuntimeSnapshot GameplayEffectRuntimeState::BuildSnapshot(
		const std::string& effectId
	) const
	{
		return GameplayEffectRuntimeSnapshot{
			effectId,
			remainingDuration,
			totalDuration,
			stackCount,
			runtimeAttributes
		};
	}
}
