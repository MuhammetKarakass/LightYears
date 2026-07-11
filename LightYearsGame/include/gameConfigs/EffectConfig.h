#pragma once

#include "gameConfigs/EffectStructs.h"

namespace EffectData
{
	static const ly::GameplayEffectDefinition BasicBarrierEffect{
		"Effect.Barrier.Basic",
		BarrierEffectSchema::BehaviorId,
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::RefreshDuration,
		5.f,
		1,
		{
			ly::GameplayTag{ "Effect.Defense.Barrier" }
		},
		{},
		{
			ly::GameplayAttribute{ BarrierEffectSchema::Capacity, 30.f, 0.f },
			ly::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
		},
		"Visual.Shield.Basic"
	};

	static const ly::GameplayEffectDefinition BarrierBreakThrustBoostEffect{
		"Effect.Test.BarrierBreak.ThrustBoost",
		{},
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::RefreshDuration,
		2.f,
		1,
		{
			ly::GameplayTag{ "Effect.Movement.Boost" },
			ly::GameplayTag{ "Effect.Test.PassiveValidation" }
		},
		{
			ly::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedVertical, ly::AttributeModifierOperation::Add, 0.25f, 0 },
			ly::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedHorizontal, ly::AttributeModifierOperation::Add, 0.2f, 0 }
		},
		{},
		""
	};

	inline const ly::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId)
	{
		if (effectId == BasicBarrierEffect.effectId)
		{
			return &BasicBarrierEffect;
		}
		if (effectId == BarrierBreakThrustBoostEffect.effectId)
		{
			return &BarrierBreakThrustBoostEffect;
		}
		return nullptr;
	}
}
