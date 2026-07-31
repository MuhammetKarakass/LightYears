#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectBehaviorResult.h"

namespace ly
{
	class Actor;
	struct DamageContext;

	namespace BarrierEffectBehavior
	{
		void AddStack(sas::ActiveGameplayEffect& effect);
		sas::GameplayEffectBehaviorResult Tick(
			sas::ActiveGameplayEffect& effect,
			Actor& owner,
			float deltaTime
		);
		sas::GameplayEffectBehaviorResult ProcessIncomingDamage(
			sas::ActiveGameplayEffect& effect,
			DamageContext& context
		);
		bool RegisterBarrierEffectBehavior();
	}
}
