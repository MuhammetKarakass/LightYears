#pragma once

#include "gameplay/effects/ActiveGameplayEffect.h"
#include "gameplay/damage/DamageContext.h"

namespace ly
{
	struct GameplayEffectBehaviorEvent
	{
		GameplayTag eventTag;
		float magnitude = 0.f;
	};

	struct GameplayEffectBehaviorResult
	{
		bool changed = false;
		bool removeEffect = false;
		List<GameplayEffectBehaviorEvent> events;
	};

	enum class IncomingDamagePhase
	{
		PreMitigation,
		Standard
	};

	namespace GameplayEffectBehavior
	{
		void Initialize(ActiveGameplayEffect& effect);
		void Refresh(ActiveGameplayEffect& effect);
		void AddStack(ActiveGameplayEffect& effect);
		GameplayEffectBehaviorResult Tick(ActiveGameplayEffect& effect, float deltaTime);
		IncomingDamagePhase GetIncomingDamagePhase(const ActiveGameplayEffect& effect);
		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context);
	}
}
