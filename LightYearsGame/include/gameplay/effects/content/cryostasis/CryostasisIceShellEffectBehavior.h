#pragma once

#include "effects/ActiveGameplayEffect.h"
#include "effects/GameplayEffectBehaviorResult.h"

namespace vega
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
