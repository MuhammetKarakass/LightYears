#pragma once

#include "gameplay/effects/GameplayEffectBehavior.h"

namespace ly::BarrierEffectBehavior
{
	void AddStack(ActiveGameplayEffect& effect);
	GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context);
}
