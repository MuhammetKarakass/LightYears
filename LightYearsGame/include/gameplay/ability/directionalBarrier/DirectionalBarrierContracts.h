#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::DirectionalBarrier
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Defense.DirectionalBarrier.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.DirectionalBarrier" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::DirectionalBarrier
	};

	// Gameplay interception and the typed presentation profile share one
	// boundary value so projectiles stop at the visible outer edge.
	inline constexpr float BarrierRadius = 105.f;

	struct State
	{
		// The effect is the canonical active-state owner for this ability.
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Effect::Defense::DirectionalBarrier::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::DirectionalBarrier::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::DirectionalBarrier::Ended
		};
	};

	struct Attribute
	{
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
