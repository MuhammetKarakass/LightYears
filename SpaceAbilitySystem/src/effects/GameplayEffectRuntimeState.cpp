#include "effects/GameplayEffectRuntimeState.h"

#include <algorithm>

namespace sas
{
	void GameplayEffectRuntimeState::Initialize(
		GameplayEffectHandle newHandle,
		float duration,
		float decayInterval,
		const GameplayAttributeList& attributes
	)
	{
		handle = newHandle;
		RefreshDuration(duration, decayInterval);
		stackCount = 1;
		appliedModifierHandles.clear();
		ResetRuntimeAttributes(attributes);
	}

	void GameplayEffectRuntimeState::RefreshDuration(float duration, float decayInterval)
	{
		remainingDuration = duration;
		totalDuration = duration;
		stackDecayInterval = decayInterval;
	}

	bool GameplayEffectRuntimeState::TickDuration(float deltaTime)
	{
		remainingDuration -= deltaTime;
		return remainingDuration <= 0.f;
	}

	bool GameplayEffectRuntimeState::TickStackDecay(float deltaTime)
	{
		if (deltaTime < 0.f || stackDecayInterval <= 0.f)
		{
			return false;
		}
		remainingDuration -= deltaTime;
		bool stackChanged = false;
		while (remainingDuration <= 0.f && stackCount > 0)
		{
			--stackCount;
			stackChanged = true;
			if (stackCount == 0)
			{
				return true;
			}
			remainingDuration += stackDecayInterval;
		}
		return stackChanged;
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
