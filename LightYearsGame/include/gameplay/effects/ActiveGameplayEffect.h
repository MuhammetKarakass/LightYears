#pragma once

#include "gameConfigs/EffectStructs.h"
#include "gameplay/effects/GameplayEffectHandle.h"
#include "framework/Core.h"

namespace ly
{
	class Actor;
	class GameplayEffectVisual;

	struct ActiveGameplayEffect
	{
		GameplayEffectHandle handle;
		GameplayEffectDefinition definition;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		List<AttributeModifierHandle> appliedModifierHandles;
		GameplayAttributeList runtimeAttributes;
		weak_ptr<GameplayEffectVisual> visual;
		Actor* source = nullptr;
	};

	struct GameplayEffectSnapshot
	{
		std::string effectId;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		GameplayAttributeList runtimeAttributes;
	};
}


