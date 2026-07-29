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
			effect.spec.attributes,
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

	GameplayEffectBehaviorResult Tick(ActiveGameplayEffect& effect, Actor& owner, float deltaTime)
	{
		(void)owner;
		GameplayEffectBehaviorResult result;
		if (deltaTime <= 0.f)
		{
			return result;
		}

		GameplayAttribute* capacity = FindGameplayAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		GameplayAttribute* regenerationDelayRemaining = FindGameplayAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::RegenerationDelayRemaining
		);
		if (!capacity || !regenerationDelayRemaining || capacity->currentValue >= capacity->baseValue)
		{
			return result;
		}

		float regenerationTime = deltaTime;
		if (regenerationDelayRemaining->currentValue > 0.f)
		{
			const float consumedDelay = std::min(regenerationDelayRemaining->currentValue, regenerationTime);
			regenerationDelayRemaining->currentValue -= consumedDelay;
			regenerationTime -= consumedDelay;
			result.changed = consumedDelay > 0.f;
		}
		if (regenerationTime <= 0.f)
		{
			return result;
		}

		const float regenerationPerSecond = std::max(
			0.f,
			FindGameplayAttributeValue(
				effect.runtimeAttributes,
				BarrierEffectSchema::RegenerationPerSecond,
				0.f
			)
		);
		if (regenerationPerSecond <= 0.f)
		{
			return result;
		}

		const float previousCapacity = capacity->currentValue;
		capacity->currentValue = std::min(
			capacity->baseValue,
			capacity->currentValue + regenerationPerSecond * regenerationTime
		);
		result.changed = result.changed || capacity->currentValue != previousCapacity;
		return result;
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
		const float shieldMultiplier = std::max(0.f, context.payload.shieldDamageMultiplier);
		if (absorptionRatio <= 0.f || shieldMultiplier <= 0.f)
		{
			return result;
		}

		// Energy damage spends more shield capacity, but never gains accidental
		// hull damage when a shield breaks part way through a hit.
		const float absorbedShieldDamage = std::min(
			capacity->currentValue,
			context.remainingDamage * absorptionRatio * shieldMultiplier
		);
		const float absorbedSourceDamage = absorbedShieldDamage / (absorptionRatio * shieldMultiplier);
		capacity->currentValue -= absorbedShieldDamage;
		context.remainingDamage = std::max(0.f, context.remainingDamage - absorbedSourceDamage);
		context.absorbedDamage += absorbedSourceDamage;
		result.changed = absorbedShieldDamage > 0.f;

		if (GameplayAttribute* regenerationDelayRemaining = FindGameplayAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::RegenerationDelayRemaining
		))
		{
			const float baseDelay = std::max(
				0.f,
				FindGameplayAttributeValue(
					effect.runtimeAttributes,
					BarrierEffectSchema::RegenerationDelay,
					0.f
				)
			);
			regenerationDelayRemaining->currentValue = std::max(
				regenerationDelayRemaining->currentValue,
				baseDelay + std::max(0.f, context.payload.shieldRegenerationDelay)
			);
			result.changed = true;
		}

		if (capacity->currentValue <= 0.f)
		{
			result.removeEffect = true;
			result.events.push_back(GameplayEffectBehaviorEvent{
				BarrierEffectSchema::BrokenEventId,
				absorbedSourceDamage
			});
		}
		return result;
	}

	bool RegisterBarrierEffectBehavior()
	{
		static const bool registered = []
		{
			GameplayEffectBehavior::Hooks hooks;
			hooks.addStack = &AddStack;
			hooks.tick = &Tick;
			hooks.processIncomingDamage = &ProcessIncomingDamage;
			return GameplayEffectBehavior::RegisterBehavior(
				BarrierEffectSchema::BehaviorId,
				hooks
			);
		}();
		return registered;
	}
}
