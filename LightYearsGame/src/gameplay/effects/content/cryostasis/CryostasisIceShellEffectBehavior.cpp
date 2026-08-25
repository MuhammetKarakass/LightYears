#include "gameplay/effects/content/cryostasis/CryostasisIceShellEffectBehavior.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/effects/EffectBehaviorKeys.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"

	#include <algorithm>

namespace ly::CryostasisIceShellEffectBehavior
{
	sas::GameplayEffectBehaviorResult ProcessIncomingDamage(
		sas::ActiveGameplayEffect& effect,
		DamageContext& context
	)
	{
		sas::GameplayEffectBehaviorResult result;
		if (context.remainingDamage <= 0.f)
		{
			return result;
		}

		sas::GameplayAttribute* iceHealth = sas::FindAttribute(
			effect.runtimeAttributes,
			AbilityData::Cryostasis::Effect::IceHealth
		);
		if (!iceHealth || iceHealth->currentValue <= 0.f)
		{
			return result;
		}

		const float incomingDamage = context.remainingDamage;
		iceHealth->currentValue = std::max(0.f, iceHealth->currentValue - incomingDamage);
		// Cryostasis consumes the entire triggering hit, even when that hit breaks
		// the shell. No excess damage may leak into shield or health.
		context.absorbedDamage += incomingDamage;
		context.remainingDamage = 0.f;
		result.changed = true;

		if (iceHealth->currentValue <= 0.f)
		{
			result.removeEffect = true;
			result.events.push_back(sas::GameplayEffectBehaviorEvent{
				AbilityData::Cryostasis::Event::IceBroken,
				incomingDamage
			});
		}
		return result;
	}

	bool RegisterCryostasisIceShellEffectBehavior()
	{
		static const bool registered = []
		{
			LightYearsEffectBehaviorRuntime::Hooks hooks;
			hooks.eventPhase = IncomingDamagePhase::PreMitigation;
			hooks.processEvent = &ProcessIncomingDamage;
			return GetEffectBehaviorRuntime().Register(
				EffectBehaviorKeys::CryostasisIceShell,
				hooks
			);
		}();
		return registered;
	}
}
