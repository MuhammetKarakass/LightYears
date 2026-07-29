#pragma once

#include "gameplay/effects/GameplayEffectHandle.h"
#include "gameplay/effects/GameplayEffectSpec.h"
#include "framework/Core.h"

#include <memory>

namespace ly
{
	class Actor;
	class GameplayEffectVisual;

	class GameplayEffectRuntimeContext
	{
	public:
		virtual ~GameplayEffectRuntimeContext() = default;
	};

	struct GameplayEffectApplicationContext
	{
		Actor* source = nullptr;
		const void* sourceScope = nullptr;
		std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext;
	};

	struct ActiveGameplayEffect
	{
		GameplayEffectHandle handle;
		GameplayEffectSpec spec;
		float remainingDuration = 0.f;
		float totalDuration = 0.f;
		int stackCount = 1;
		List<AttributeModifierHandle> appliedModifierHandles;
		GameplayAttributeList runtimeAttributes;
		weak_ptr<GameplayEffectVisual> visual;
		Actor* source = nullptr;
		const void* sourceScope = nullptr;
		std::shared_ptr<GameplayEffectRuntimeContext> runtimeContext;
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


