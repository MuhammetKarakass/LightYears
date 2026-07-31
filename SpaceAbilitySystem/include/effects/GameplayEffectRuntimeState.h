#pragma once

#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectHandle.h"
#include "effects/GameplayEffectRuntimeSnapshot.h"

#include <string>

namespace sas
{
	struct GameplayEffectRuntimeState
	{
		GameplayEffectHandle handle;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		ly::List<AttributeModifierHandle> appliedModifierHandles;
		GameplayAttributeList runtimeAttributes;

		void Initialize(
			GameplayEffectHandle newHandle,
			float duration,
			const GameplayAttributeList& attributes
		);
		void RefreshDuration(float duration);
		bool TickDuration(float deltaTime);
		void ResetStackCount();
		bool TryAddStack(int maxStacks);
		void ResetRuntimeAttributes(const GameplayAttributeList& attributes);
		GameplayEffectRuntimeSnapshot BuildSnapshot(const std::string& effectId) const;
	};
}
