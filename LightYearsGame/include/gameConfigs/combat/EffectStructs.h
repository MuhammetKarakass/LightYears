#pragma once

#include "framework/Core.h"
#include "effects/GameplayEffectDefinition.h"
#include "gameplay/tags/GameplayTags.h"

struct BarrierEffectSchema
{
	inline static constexpr char BasicEffectId[] = "Effect.Barrier.Basic";
	inline static constexpr char BreakThrustBoostEffectId[] =
		"Effect.Test.BarrierBreak.ThrustBoost";
	inline static const ly::GameplayTag& GrantedTag = ly::GameplayTags::State::Effect::Defense::Barrier;
	inline static const ly::GameplayTag& BreakThrustBoostTag = ly::GameplayTags::State::Effect::Movement::Boost;
	inline static const ly::GameplayTag& BreakValidationTag = ly::GameplayTags::State::Effect::Test::PassiveValidation;
	inline static const sas::AttributeId Capacity{ "Effect.BarrierCapacity" };
	inline static const sas::AttributeId AbsorptionRatio{ "Effect.BarrierAbsorptionRatio" };
	inline static const sas::AttributeId RegenerationPerSecond{ "Effect.BarrierRegenerationPerSecond" };
	inline static const sas::AttributeId RegenerationDelay{ "Effect.BarrierRegenerationDelay" };
	inline static const sas::AttributeId RegenerationDelayRemaining{ "Effect.BarrierRegenerationDelayRemaining" };
	inline static const ly::GameplayTag& BrokenEventTag = ly::GameplayTags::Event::Owner::BarrierBroken;
};

struct MovementEffectSchema
{
	inline static constexpr char SlowEffectId[] = "Effect.Movement.Slow.Basic";
	inline static constexpr char SlowImmunityEffectId[] = "Effect.Immunity.Movement.Slow";
	inline static const ly::GameplayTag& SlowGrantedTag = ly::GameplayTags::State::Effect::Movement::Slow;
	inline static const ly::GameplayTag& SlowImmunityGrantedTag = ly::GameplayTags::State::Effect::Immunity::Movement::Slow;
};

namespace EffectData
{
	const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId);
}
