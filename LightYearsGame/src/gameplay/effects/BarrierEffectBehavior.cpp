#include "gameplay/effects/BarrierEffectBehavior.h"
#include <algorithm>

namespace ly::BarrierEffectBehavior
{
	void AddStack(ActiveGameplayEffect& effect)
	{
		GameplayAttribute* runtimeCapacity = FindGameplayAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		const GameplayAttribute* stackCapacity = FindGameplayAttribute(
			effect.definition.attributes,
			BarrierEffectSchema::Capacity
		);
		if (!runtimeCapacity || !stackCapacity)
		{
			return;
		}
		runtimeCapacity->baseValue += stackCapacity->baseValue;
		runtimeCapacity->currentValue = std::min(
			runtimeCapacity->maxValue,
			runtimeCapacity->currentValue + stackCapacity->baseValue
		);
	}

	GameplayEffectBehaviorResult ProcessIncomingDamage(ActiveGameplayEffect& effect, DamageContext& context)
	{
		GameplayEffectBehaviorResult result;
		if (context.remainingDamage <= 0.f)
		{
			return result;
		}

		GameplayAttribute* capacity = FindGameplayAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		if (!capacity || capacity->currentValue <= 0.f)
		{
			return result;
		}

		const float absorptionRatio = std::clamp(
			FindGameplayAttributeValue(
				effect.runtimeAttributes,
				BarrierEffectSchema::AbsorptionRatio,
				1.f
			),
			0.f,
			1.f
		);
		const float absorbed = std::min(
			capacity->currentValue,
			context.remainingDamage * absorptionRatio
		);
		capacity->currentValue -= absorbed;
		context.remainingDamage -= absorbed;
		context.absorbedDamage += absorbed;
		result.changed = absorbed > 0.f;

		if (capacity->currentValue <= 0.f)
		{
			result.removeEffect = true;
			result.events.push_back(GameplayEffectBehaviorEvent{
				BarrierEffectSchema::BrokenEventId,
				absorbed
			});
		}
		return result;
	}
}
