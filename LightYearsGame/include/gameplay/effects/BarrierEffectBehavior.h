#pragma once

#include "gameplay/effects/GameplayEffectBehavior.h"

namespace ly::BarrierEffectBehavior
{
	void AddStack(ActiveGameplayEffect& effect);
	GameplayEffectBehaviorResult Tick(ActiveGameplayEffect& effect, float deltaTime);
	GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context);
}
