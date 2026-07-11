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

	namespace GameplayEffectBehavior
	{
		void Initialize(ActiveGameplayEffect& effect);
		void Refresh(ActiveGameplayEffect& effect);
		void AddStack(ActiveGameplayEffect& effect);
		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context);
	}
}
