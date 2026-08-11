#include "attributes/AttributeSystem.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/effects/content/barrier/BarrierEffectBehavior.h"
#include <algorithm>

namespace ly::BarrierEffectBehavior
{
	void AddStack(sas::ActiveGameplayEffect& effect)
	{
		sas::GameplayAttribute* runtimeCapacity = sas::FindAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		const sas::GameplayAttribute* stackCapacity = sas::FindAttribute(
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

	sas::GameplayEffectBehaviorResult Tick(
		sas::ActiveGameplayEffect& effect,
		Actor& owner,
		float deltaTime
	)
	{
		(void)owner;
		sas::GameplayEffectBehaviorResult result;
		if (deltaTime <= 0.f)
		{
			return result;
		}

		sas::GameplayAttribute* capacity = sas::FindAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		sas::GameplayAttribute* regenerationDelayRemaining = sas::FindAttribute(
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
			sas::FindAttributeValue(
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

		sas::GameplayAttribute* capacity = sas::FindAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::Capacity
		);
		if (!capacity || capacity->currentValue <= 0.f)
		{
			return result;
		}

		const float absorptionRatio = std::clamp(
			sas::FindAttributeValue(
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

		if (sas::GameplayAttribute* regenerationDelayRemaining = sas::FindAttribute(
			effect.runtimeAttributes,
			BarrierEffectSchema::RegenerationDelayRemaining
		))
		{
			const float baseDelay = std::max(
				0.f,
				sas::FindAttributeValue(
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
			result.events.push_back(sas::GameplayEffectBehaviorEvent{
				BarrierEffectSchema::BrokenEventTag,
				absorbedSourceDamage
			});
		}
		return result;
	}

	bool RegisterBarrierEffectBehavior()
	{
		static const bool registered = []
		{
			LightYearsEffectBehaviorRuntime::Hooks hooks;
			hooks.addStack = &AddStack;
			hooks.tick = &Tick;
			hooks.eventPhase =
				IncomingDamagePhase::Standard;
			hooks.processEvent = &ProcessIncomingDamage;
			return GetEffectBehaviorRuntime().Register(
				EffectData::BarrierBehaviorKey,
				hooks
			);
		}();
		return registered;
	}
}
