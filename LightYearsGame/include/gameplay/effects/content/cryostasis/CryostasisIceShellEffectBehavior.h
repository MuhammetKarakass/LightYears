#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectBehaviorResult.h"

namespace ly
{
	struct DamageContext;

	namespace CryostasisIceShellEffectBehavior
	{
		sas::GameplayEffectBehaviorResult ProcessIncomingDamage(
			sas::ActiveGameplayEffect& effect,
			DamageContext& context
		);
		bool RegisterCryostasisIceShellEffectBehavior();
	}
}
