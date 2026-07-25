#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/effects/BarrierEffectBehavior.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include <algorithm>

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

		GameplayEffectBehaviorResult Tick(ActiveGameplayEffect& effect, float deltaTime)
		{
			if (effect.definition.behaviorTag == BarrierEffectSchema::BehaviorId)
			{
				return BarrierEffectBehavior::Tick(effect, deltaTime);
			}
			return {};
		}

		IncomingDamagePhase GetIncomingDamagePhase(const ActiveGameplayEffect& effect)
		{
			return effect.definition.behaviorTag == DamageStatusSchema::ElectricBehavior
				? IncomingDamagePhase::PreMitigation
				: IncomingDamagePhase::Standard;
		}

		GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context)
		{
			if (effect.definition.behaviorTag == DamageStatusSchema::ElectricBehavior)
			{
				if (effect.stackCount < std::max(1, effect.definition.maxStacks))
				{
					return {};
				}
				const float multiplierPerStack = std::max(0.f, FindGameplayAttributeValue(
					effect.runtimeAttributes,
					DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
					0.f
				));
				context.remainingDamage *= 1.f + multiplierPerStack * static_cast<float>(effect.stackCount);
				return {};
			}
			if (effect.definition.behaviorTag == BarrierEffectSchema::BehaviorId)
			{
				return BarrierEffectBehavior::ProcessIncomingDamage(effect, context);
			}
			return {};
		}
	}
}
