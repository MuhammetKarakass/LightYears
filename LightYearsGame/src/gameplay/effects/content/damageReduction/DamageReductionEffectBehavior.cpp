#include "gameplay/effects/content/damageReduction/DamageReductionEffectBehavior.h"

#include "attributes/AttributeSystem.h"
#include "gameConfigs/combat/EffectStructs.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/effects/EffectBehaviorKeys.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

#include <algorithm>

namespace ly::DamageReductionEffectBehavior
{
	static sas::GameplayEffectBehaviorResult ProcessIncomingDamage(
		sas::ActiveGameplayEffect& effect,
		DamageContext& context
	)
	{
		sas::GameplayEffectBehaviorResult result;
		if (context.remainingDamage <= 0.f)
		{
			return result;
		}
		const float reduction = std::clamp(
			sas::FindAttributeValue(
				effect.runtimeAttributes,
				DamageReductionEffectSchema::Fraction,
				0.f
			),
			0.f,
			0.95f
		);
		if (reduction <= 0.f)
		{
			return result;
		}
		const float before = context.remainingDamage;
		context.remainingDamage *= 1.f - reduction;
		context.mitigatedDamage += before - context.remainingDamage;
		context.modifiedDamage = context.remainingDamage;
		result.changed = true;
		return result;
	}

	bool RegisterDamageReductionEffectBehavior()
	{
		static const bool registered = []
		{
			LightYearsEffectBehaviorRuntime::Hooks hooks;
			hooks.eventPhase = IncomingDamagePhase::PreMitigation;
			hooks.processEvent = &ProcessIncomingDamage;
			return GetEffectBehaviorRuntime().Register(
				EffectBehaviorKeys::DamageReduction,
				hooks
			);
		}();
		return registered;
	}
}
