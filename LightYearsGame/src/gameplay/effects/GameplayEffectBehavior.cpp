#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/effects/BarrierEffectBehavior.h"

namespace ly
{
	namespace
	{
		void ResetRuntimeAttributes(ActiveGameplayEffect& effect)
		{
			effect.runtimeAttributes = effect.definition.attributes;
			for (GameplayAttribute& attribute : effect.runtimeAttributes)
			{
				attribute.currentValue = attribute.baseValue;
			}
		}

	}

	namespace GameplayEffectBehavior
	{
		void Initialize(ActiveGameplayEffect& effect)
		{
			ResetRuntimeAttributes(effect);
		}

		void Refresh(ActiveGameplayEffect& effect)
		{
			ResetRuntimeAttributes(effect);
		}

		void AddStack(ActiveGameplayEffect& effect)
		{
			if (effect.definition.behaviorTag == BarrierEffectSchema::BehaviorId)
			{
				BarrierEffectBehavior::AddStack(effect);
			}
		}

		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context)
		{
			if (effect.definition.behaviorTag == BarrierEffectSchema::BehaviorId)
			{
				return BarrierEffectBehavior::ProcessIncomingDamage(effect, context);
			}
			return {};
		}
	}
}
