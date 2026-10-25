#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>

namespace AbilityData::DirectionalBarrier
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Defense.DirectionalBarrier.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Defense };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::DirectionalBarrier
	};

	// Gameplay interception and the typed presentation profile share one
	// boundary value so projectiles stop at the visible outer edge.
	inline constexpr float BarrierRadius = 105.f;
	inline float ResolveHalfAngleDegrees(float mainEffectMultiplier) { return std::clamp(60.f * mainEffectMultiplier, 0.f, 90.f); }

	struct State
	{
		// The effect is the canonical active-state owner for this ability.
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Effect::Defense::DirectionalBarrier::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::DirectionalBarrier::Started
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::DirectionalBarrier::Ended
		};
	};

	struct Attribute
	{
		// Activation snapshot shared by damage interception and the visible arc.
		inline static const sas::AttributeId HalfAngleDegrees{ "Ability.Defense.DirectionalBarrier.HalfAngleDegrees" };
		// Duration scaling reads the owner's MaxHealth. These are ability-local
		// tuning attributes; OwnerAttributeIds::MaxHealth remains the runtime
		// source attribute and is not duplicated in the ability definition.
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Defense.DirectionalBarrier.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthDurationScale{
			"Ability.Defense.DirectionalBarrier.MaxHealthDurationScale"
		};
		inline static const sas::AttributeId MovementSpeedMultiplier{
			"Ability.Defense.DirectionalBarrier.MovementSpeedMultiplier"
		};
	};

	struct Effect
	{
		inline static constexpr char ActiveEffectId[] =
			"Effect.Defense.DirectionalBarrier.Active.Basic";
	};
}
