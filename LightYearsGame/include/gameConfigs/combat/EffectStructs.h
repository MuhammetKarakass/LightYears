#pragma once

#include "framework/Core.h"
#include "effects/GameplayEffectDefinition.h"

struct BarrierEffectSchema
{
	inline static const ly::GameplayTag BehaviorId{ "EffectBehavior.Barrier" };
	inline static const ly::GameplayTag Capacity{ "Attribute.Effect.BarrierCapacity" };
	inline static const ly::GameplayTag AbsorptionRatio{ "Attribute.Effect.BarrierAbsorptionRatio" };
	inline static const ly::GameplayTag RegenerationPerSecond{ "Attribute.Effect.BarrierRegenerationPerSecond" };
	inline static const ly::GameplayTag RegenerationDelay{ "Attribute.Effect.BarrierRegenerationDelay" };
	inline static const ly::GameplayTag RegenerationDelayRemaining{ "Attribute.Effect.BarrierRegenerationDelayRemaining" };
	inline static const ly::GameplayTag BrokenEventId{ "Event.Owner.BarrierBroken" };
};

namespace EffectData
{
	const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId);
}
