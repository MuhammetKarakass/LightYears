#pragma once

#include "framework/Core.h"
#include "effects/GameplayEffectDefinition.h"

struct BarrierEffectSchema
{
	inline static constexpr char BasicEffectId[] = "Effect.Barrier.Basic";
	inline static constexpr char BreakThrustBoostEffectId[] =
		"Effect.Test.BarrierBreak.ThrustBoost";
	inline static const ly::GameplayTag GrantedTag{ "State.Effect.Defense.Barrier" };
	inline static const ly::GameplayTag BreakThrustBoostTag{ "State.Effect.Movement.Boost" };
	inline static const ly::GameplayTag BreakValidationTag{ "State.Effect.Test.PassiveValidation" };
	inline static const ly::GameplayTag BehaviorTag{ "EffectBehavior.Barrier" };
	inline static const ly::GameplayTag Capacity{ "Attribute.Effect.BarrierCapacity" };
	inline static const ly::GameplayTag AbsorptionRatio{ "Attribute.Effect.BarrierAbsorptionRatio" };
	inline static const ly::GameplayTag RegenerationPerSecond{ "Attribute.Effect.BarrierRegenerationPerSecond" };
	inline static const ly::GameplayTag RegenerationDelay{ "Attribute.Effect.BarrierRegenerationDelay" };
	inline static const ly::GameplayTag RegenerationDelayRemaining{ "Attribute.Effect.BarrierRegenerationDelayRemaining" };
	inline static const ly::GameplayTag BrokenEventTag{ "Event.Owner.BarrierBroken" };
};

struct MovementEffectSchema
{
	inline static constexpr char SlowEffectId[] = "Effect.Movement.Slow.Basic";
	inline static const ly::GameplayTag SlowGrantedTag{ "State.Effect.Movement.Slow" };
};

namespace EffectData
{
	const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId);
}
