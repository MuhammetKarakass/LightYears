#pragma once

#include "framework/Core.h"
#include "effects/GameplayEffectDefinition.h"
#include "gameplay/tags/GameplayTags.h"

struct BarrierEffectSchema
{
	inline static constexpr char BasicEffectId[] = "Effect.Barrier.Basic";
	inline static constexpr char BreakThrustBoostEffectId[] =
		"Effect.Test.BarrierBreak.ThrustBoost";
	inline static const vega::GameplayTag& GrantedTag = vega::GameplayTags::State::Effect::Defense::Barrier;
	inline static const vega::GameplayTag& BreakThrustBoostTag = vega::GameplayTags::State::Effect::Movement::Boost;
	inline static const vega::GameplayTag& BreakValidationTag = vega::GameplayTags::State::Effect::Test::PassiveValidation;
	inline static const sas::AttributeId Capacity{ "Effect.BarrierCapacity" };
	inline static const sas::AttributeId AbsorptionRatio{ "Effect.BarrierAbsorptionRatio" };
	inline static const sas::AttributeId RegenerationPerSecond{ "Effect.BarrierRegenerationPerSecond" };
	inline static const sas::AttributeId RegenerationDelay{ "Effect.BarrierRegenerationDelay" };
	inline static const sas::AttributeId RegenerationDelayRemaining{ "Effect.BarrierRegenerationDelayRemaining" };
	inline static const vega::GameplayTag& BrokenEventTag = vega::GameplayTags::Event::Owner::BarrierBroken;
};

struct MovementEffectSchema
{
	inline static constexpr char SlowEffectId[] = "Effect.Movement.Slow.Basic";
	inline static constexpr char SlowImmunityEffectId[] = "Effect.Immunity.Movement.Slow";
	inline static const vega::GameplayTag& SlowGrantedTag = vega::GameplayTags::State::Effect::Movement::Slow;
	inline static const vega::GameplayTag& SlowImmunityGrantedTag = vega::GameplayTags::State::Effect::Immunity::Movement::Slow;
};

// Reusable incoming-damage multiplier. Effects author a resolved fraction in
// their spec, while the behavior applies it consistently in PreMitigation.
struct DamageReductionEffectSchema
{
	inline static const sas::AttributeId Fraction{
		"Effect.Defense.DamageReduction.Fraction"
	};
};

namespace EffectData
{
	const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId);
}
