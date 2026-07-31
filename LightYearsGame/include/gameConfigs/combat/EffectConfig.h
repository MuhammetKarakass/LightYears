#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/combat/EffectStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/ability/GravityAnomalyConfig.h"
#include "presentation/effects/shield/ShieldVisualIds.h"

namespace EffectData
{
	inline const sas::GameplayEffectDefinition BasicBarrierEffect{
		"Effect.Barrier.Basic",
		BarrierEffectSchema::BehaviorId,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		5.f,
		1,
		{
			ly::GameplayTag{ "Effect.Defense.Barrier" }
		},
		{},
		{
			sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 30.f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationPerSecond, 6.f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 1.5f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f }
		},
		ly::ShieldVisualIds::Basic
	};

	inline const sas::GameplayEffectDefinition BarrierBreakThrustBoostEffect{
		"Effect.Test.BarrierBreak.ThrustBoost",
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		2.f,
		1,
		{
			ly::GameplayTag{ "Effect.Movement.Boost" },
			ly::GameplayTag{ "Effect.Test.PassiveValidation" }
		},
		{
			sas::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedVertical, sas::AttributeModifierOperation::Add, 0.25f, 0 },
			sas::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedHorizontal, sas::AttributeModifierOperation::Add, 0.2f, 0 }
		},
		{},
		""
	};

	inline const sas::GameplayEffectDefinition IgniteEffect{
		"Effect.Status.Damage.Ignite",
		ly::DamageStatusSchema::IgniteBehavior,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		3.f,
		4,
		{ ly::DamageStatusSchema::Ignite },
		{},
		{
			sas::GameplayAttribute{
				ly::DamageAttributeIds::BurnDamagePerSecond,
				1.f,
				0.f
			}
		}
	};

	inline const sas::GameplayEffectDefinition CryoBuildupEffect{
		ly::DamageStatusEffectIds::CryoBuildup,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		2.5f,
		4,
		{ ly::DamageStatusSchema::CryoBuildup }
	};

	inline const sas::GameplayEffectDefinition CryoSlowEffect{
		ly::DamageStatusEffectIds::CryoSlowed,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		1.5f,
		1,
		{ ly::DamageStatusSchema::CryoSlowed },
		{
			sas::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				0.25f
			}
		}
	};

	inline const sas::GameplayEffectDefinition ElectricEffect{
		"Effect.Status.Damage.Electric",
		ly::DamageStatusSchema::ElectricBehavior,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		3.f,
		4,
		{ ly::DamageStatusSchema::Electric },
		{},
		{
			sas::GameplayAttribute{
				ly::DamageAttributeIds::ElectricDamageTakenMultiplierPerStack,
				0.04f,
				0.f
			}
		}
	};

	inline const sas::GameplayEffectDefinition GravityAnomalyInsideEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId =
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId;
		definition.behaviorTag =
			AbilityData::GravityAnomaly::EffectSchema::BehaviorId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy =
			sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration =
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds;
		definition.maxStacks = 1;
		definition.grantedTags = {
			AbilityData::GravityAnomaly::EffectSchema::InsideTag
		};
		definition.modifiers = {
			sas::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				AbilityData::GravityAnomaly::BasicSettings.slowMagnitude
			}
		};
		definition.activeVisualId = "Visual.Effect.GravityAnomaly.Inside";
		definition.sourceScopedApplication = true;
		return definition;
	}();

	inline const ly::List<const sas::GameplayEffectDefinition*>&
	GetShippedGameplayEffectDefinitions()
	{
		static const ly::List<const sas::GameplayEffectDefinition*> definitions{
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

	inline const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId)
	{
		for (const sas::GameplayEffectDefinition* definition :
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
