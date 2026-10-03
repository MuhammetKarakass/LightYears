#pragma once

#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectHandle.h"
#include "effects/GameplayEffectRuntimeSnapshot.h"

#include <cstddef>
#include <string>

namespace sas
{
	struct GameplayEffectRuntimeState
	{
		GameplayEffectHandle handle;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		float stackDecayInterval = 1.f;
		int stackCount = 1;
		ly::List<AttributeModifierHandle> appliedModifierHandles;
		ly::List<ly::GameplayTag> appliedGrantedTags;
		GameplayAttributeList runtimeAttributes;
		// Owned by the runtime system's operation scope; callbacks must not touch these.
		bool operationActive = false;
		bool removeRequested = false;
		bool removalInProgress = false;

		bool IsRemovalPending() const { return removeRequested || removalInProgress; }
		bool CanBeginOperation() const { return !operationActive && !IsRemovalPending(); }
		// Removal was requested mid-operation and has not started: applications must not stack onto it.
		bool IsAwaitingDeferredRemoval() const { return operationActive && removeRequested && !removalInProgress; }

		void Initialize(
			GameplayEffectHandle newHandle,
			float duration,
			float decayInterval,
			const GameplayAttributeList& attributes
		);
		void RefreshDuration(float duration, float decayInterval = 1.f);
		bool TickDuration(float deltaTime);
		bool TickStackDecay(float deltaTime);
		void ResetStackCount();
		bool TryAddStack(int maxStacks);
		void ResetRuntimeAttributes(const GameplayAttributeList& attributes);
		GameplayEffectRuntimeSnapshot BuildSnapshot(const std::string& effectId) const;
	};
}
