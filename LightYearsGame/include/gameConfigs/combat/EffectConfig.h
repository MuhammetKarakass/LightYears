#pragma once

#include "gameConfigs/combat/EffectStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/ability/GravityAnomalyConfig.h"
#include "presentation/effects/shield/ShieldVisualIds.h"

namespace EffectData
{
	inline const ly::GameplayEffectDefinition BasicBarrierEffect{
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
			ly::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
			ly::GameplayAttribute{ BarrierEffectSchema::RegenerationPerSecond, 6.f, 0.f },
			ly::GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 1.5f, 0.f },
			ly::GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f }
		},
		ly::ShieldVisualIds::Basic
	};

	inline const ly::GameplayEffectDefinition BarrierBreakThrustBoostEffect{
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

	inline const ly::GameplayEffectDefinition IgniteEffect{
		"Effect.Status.Damage.Ignite",
		ly::DamageStatusSchema::IgniteBehavior,
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::Stack,
		3.f,
		4,
		{ ly::DamageStatusSchema::Ignite },
		{},
		{
			ly::GameplayAttribute{
				ly::DamageAttributeIds::BurnDamagePerSecond,
				1.f,
				0.f
			}
		}
	};

	inline const ly::GameplayEffectDefinition CryoBuildupEffect{
		ly::DamageStatusEffectIds::CryoBuildup,
		{},
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::Stack,
		2.5f,
		4,
		{ ly::DamageStatusSchema::CryoBuildup }
	};

	inline const ly::GameplayEffectDefinition CryoSlowEffect{
		ly::DamageStatusEffectIds::CryoSlowed,
		{},
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::RefreshDuration,
		1.5f,
		1,
		{ ly::DamageStatusSchema::CryoSlowed },
		{
			ly::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				ly::AttributeModifierOperation::Add,
				0.25f
			}
		}
	};

	inline const ly::GameplayEffectDefinition ElectricEffect{
		"Effect.Status.Damage.Electric",
		ly::DamageStatusSchema::ElectricBehavior,
		ly::GameplayEffectDurationPolicy::Duration,
		ly::GameplayEffectStackingPolicy::Stack,
		3.f,
		4,
		{ ly::DamageStatusSchema::Electric },
		{},
		{
			ly::GameplayAttribute{
				ly::DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
				0.04f,
				0.f
			}
		}
	};

	inline const ly::GameplayEffectDefinition GravityAnomalyInsideEffect = []
	{
		ly::GameplayEffectDefinition definition;
		definition.effectId =
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId;
		definition.behaviorTag =
			AbilityData::GravityAnomaly::EffectSchema::BehaviorId;
		definition.durationPolicy = ly::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy =
			ly::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration =
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds;
		definition.maxStacks = 1;
		definition.grantedTags = {
			AbilityData::GravityAnomaly::EffectSchema::InsideTag
		};
		definition.modifiers = {
			ly::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				ly::AttributeModifierOperation::Add,
				AbilityData::GravityAnomaly::BasicSettings.slowMagnitude
			}
		};
		definition.activeVisualId = "Visual.Effect.GravityAnomaly.Inside";
		definition.sourceScopedApplication = true;
		return definition;
	}();

	inline const ly::List<const ly::GameplayEffectDefinition*>&
	GetShippedGameplayEffectDefinitions()
	{
		static const ly::List<const ly::GameplayEffectDefinition*> definitions{
			&BasicBarrierEffect,
			&BarrierBreakThrustBoostEffect,
			&IgniteEffect,
			&CryoBuildupEffect,
			&CryoSlowEffect,
			&ElectricEffect,
			&GravityAnomalyInsideEffect
		};
		return definitions;
	}

	inline const ly::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId)
	{
		for (const ly::GameplayEffectDefinition* definition :
			GetShippedGameplayEffectDefinitions())
		{
			if (definition && definition->effectId == effectId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}
