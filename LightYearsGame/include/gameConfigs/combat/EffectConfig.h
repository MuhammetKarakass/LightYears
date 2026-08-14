#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/combat/EffectStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/effects/EffectBehaviorKeys.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualIds.h"

namespace EffectData
{
	// Effect behavior selectors belong to the content/registry boundary, not to
	// gameplay schemas or ability contracts.
	inline const sas::GameplayEffectBehaviorKey& BarrierBehaviorKey =
		ly::EffectBehaviorKeys::Barrier;
	inline const sas::GameplayEffectBehaviorKey& DamageIgniteBehaviorKey =
		ly::EffectBehaviorKeys::DamageIgnite;
	inline const sas::GameplayEffectBehaviorKey& DamageElectricBehaviorKey =
		ly::EffectBehaviorKeys::DamageElectric;
	inline const sas::GameplayEffectBehaviorKey& GravityAnomalyBehaviorKey =
		ly::EffectBehaviorKeys::GravityAnomaly;

	// JSON fallback/test catalog only. Shipped runtime policy comes from
	// effects.json; source-owned magnitudes and durations come from the weapon,
	// ability, attachment, enemy or reward that creates GameplayEffectSpec.
	inline const sas::GameplayEffectDefinition BasicBarrierEffect{
		BarrierEffectSchema::BasicEffectId,
		BarrierBehaviorKey,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		5.f,
		1,
		{
			BarrierEffectSchema::GrantedTag
		},
		{},
		{
			sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 30.f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationPerSecond, 6.f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 1.5f, 0.f },
			sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f }
		},
		ly::ShieldVisualIds::Basic,
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Beneficial,
		false,
		"Defense.Barrier",
		""
	};

	inline const sas::GameplayEffectDefinition BarrierBreakThrustBoostEffect{
		BarrierEffectSchema::BreakThrustBoostEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		2.f,
		1,
		{
			BarrierEffectSchema::BreakThrustBoostTag,
			BarrierEffectSchema::BreakValidationTag
		},
		{
			sas::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedVertical, sas::AttributeModifierOperation::Add, 0.25f, 0 },
			sas::AttributeModifier{ ly::OwnerAttributeIds::MoveSpeedHorizontal, sas::AttributeModifierOperation::Add, 0.2f, 0 }
		},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Beneficial,
		false,
		"AttributeModifier",
		""
	};

	inline const sas::GameplayEffectDefinition IgniteEffect{
		ly::DamageStatusEffectIds::IgniteEffectId,
		DamageIgniteBehaviorKey,
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
		},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Damage.OverTime",
		""
	};

	inline const sas::GameplayEffectDefinition CryoBuildupEffect{
		ly::DamageStatusEffectIds::CryoBuildupEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		2.5f,
		4,
		{ ly::DamageStatusSchema::CryoBuildup },
		{},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Control.CryoBuildup",
		""
	};

	inline const sas::GameplayEffectDefinition CryoSlowEffect{
		ly::DamageStatusEffectIds::CryoSlowedEffectId,
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
		},
		{},
		"",
		{},
		{ MovementEffectSchema::SlowImmunityGrantedTag },
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Movement.Slow",
		"Movement.Slow"
	};

	inline const sas::GameplayEffectDefinition ElectricEffect{
		ly::DamageStatusEffectIds::ElectricEffectId,
		DamageElectricBehaviorKey,
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
		},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Damage.Electric",
		""
	};

	inline const sas::GameplayEffectDefinition GravityAnomalyInsideEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId =
			AbilityData::GravityAnomaly::Effect::InsideEffectId;
	definition.behaviorKey =
			GravityAnomalyBehaviorKey;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy =
			sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration =
			AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds;
		definition.maxStacks = 1;
		definition.grantedTags = {
			ly::GameplayTags::State::Effect::GravityAnomaly::Inside
		};
		definition.applicationBlockedTags = {
			MovementEffectSchema::SlowImmunityGrantedTag
		};
		definition.modifiers = {
			sas::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				// GravityAnomalyFieldActor resolves this value from the JSON-loaded
				// actor attributes before applying the effect spec.
				0.f
			}
		};
		definition.activeVisualId = ly::GravityAnomalyEffectVisualIds::Inside;
		definition.sourceScopedApplication = true;
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.cleanseable = true;
		definition.category = "Control.GravityAnomaly";
		definition.immunityCategory = "Movement.Slow";
		return definition;
	}();

	inline const sas::GameplayEffectDefinition MovementSlowEffect{
		MovementEffectSchema::SlowEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		3.0f,
		1,
		{
			MovementEffectSchema::SlowGrantedTag
		},
		{
			sas::AttributeModifier{
				ly::OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				0.35f,
				0
			}
		},
		{},
		"",
		{},
		{ MovementEffectSchema::SlowImmunityGrantedTag },
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Movement.Slow",
		"Movement.Slow"
	};

	inline const sas::GameplayEffectDefinition MovementSlowImmunityEffect{
		MovementEffectSchema::SlowImmunityEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::RefreshDuration,
		5.f,
		1,
		{ MovementEffectSchema::SlowImmunityGrantedTag },
		{},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Beneficial,
		false,
		"Immunity.MovementSlow",
		"",
		"Movement.Slow"
	};

	// Overdrive supplies duration and the AttackSpeed magnitude at application
	// time. The fallback exists only so the JSON effect can resolve a typed
	// policy definition without introducing a second effect behavior class.
	inline const sas::GameplayEffectDefinition OverdriveCoreAttackSpeedBoostEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::OverdriveCore::Effect::AttackSpeedBoostId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.sourceParameterized = true;
		definition.grantedTags = {
			ly::GameplayTags::State::Effect::OverdriveCore::AttackSpeedBoost
		};
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Offense.OverdriveCore.AttackSpeedBoost";
		return definition;
	}();

	// Execution Drive replaces this source-scoped effect whenever its kill
	// streak changes. The ability supplies the current AttackPower magnitude and
	// the five-second duration through GameplayEffectSpec.
	inline const sas::GameplayEffectDefinition ExecutionDriveAttackPowerEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::ExecutionDrive::Effect::AttackPowerId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.sourceScopedApplication = true;
		definition.sourceParameterized = true;
		definition.grantedTags = {
			ly::GameplayTags::State::Effect::Offense::ExecutionDrive::AttackPower
		};
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Offense.ExecutionDrive.AttackPower";
		return definition;
	}();

	// Phase Drift owns the actual movement/resource multipliers in the reusable
	// ship runtime modifier set. These policy effects provide source-scoped
	// lifecycle records so the ability can track and remove only its own effects.
	inline const sas::GameplayEffectDefinition PhaseDriftMovementBoostEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::PhaseDrift::Effect::MovementBoostId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration = 6.f;
		definition.maxStacks = 1;
		definition.grantedTags = { ly::GameplayTags::State::Effect::Movement::Boost };
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.PhaseDrift.Movement";
		definition.sourceScopedApplication = true;
		return definition;
	}();

	inline const sas::GameplayEffectDefinition PhaseDriftShieldRecoveryEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::PhaseDrift::Effect::ShieldRecoveryId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration = 6.f;
		definition.maxStacks = 1;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.PhaseDrift.ShieldRecovery";
		definition.sourceScopedApplication = true;
		return definition;
	}();

	inline const sas::GameplayEffectDefinition PhaseDriftAfterburnerRecoveryEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::PhaseDrift::Effect::AfterburnerRecoveryId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration = 6.f;
		definition.maxStacks = 1;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.PhaseDrift.AfterburnerRecovery";
		definition.sourceScopedApplication = true;
		return definition;
	}();

	// Control effects are policy-only definitions. Null Pulse supplies the
	// resolved duration and target-side response at application time.
	inline const sas::GameplayEffectDefinition NullPulseStunEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::NullPulse::Effect::StunId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.grantedTags = {
			ly::GameplayTags::State::Effect::Control::Stunned
		};
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.cleanseable = true;
		definition.category = "Control.Stun";
		definition.immunityCategory = "Control.Stun";
		definition.sourceParameterized = true;
		return definition;
	}();

	inline const sas::GameplayEffectDefinition NullPulseStaggerEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::NullPulse::Effect::StaggerId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.grantedTags = {
			ly::GameplayTags::State::Effect::Control::Staggered
		};
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.cleanseable = true;
		definition.category = "Control.Stagger";
		definition.immunityCategory = "Control.Stagger";
		definition.sourceParameterized = true;
		return definition;
	}();

	inline const ly::List<const sas::GameplayEffectDefinition*>&
	GetBuiltinGameplayEffectDefinitions()
	{
		static const ly::List<const sas::GameplayEffectDefinition*> definitions{
			&BasicBarrierEffect,
			&BarrierBreakThrustBoostEffect,
			&IgniteEffect,
			&CryoBuildupEffect,
			&CryoSlowEffect,
			&ElectricEffect,
			&GravityAnomalyInsideEffect,
			&MovementSlowEffect,
			&MovementSlowImmunityEffect,
			&OverdriveCoreAttackSpeedBoostEffect,
			&ExecutionDriveAttackPowerEffect,
			&PhaseDriftMovementBoostEffect,
			&PhaseDriftShieldRecoveryEffect,
			&PhaseDriftAfterburnerRecoveryEffect,
			&NullPulseStunEffect,
			&NullPulseStaggerEffect
		};
		return definitions;
	}

	inline const ly::List<const sas::GameplayEffectDefinition*>&
	GetShippedGameplayEffectDefinitions()
	{
		if (ly::content::EffectContentCatalog::IsLoaded())
		{
			return ly::content::EffectContentCatalog::GetDefinitions();
		}
		return GetBuiltinGameplayEffectDefinitions();
	}

	inline const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId)
	{
		if (ly::content::EffectContentCatalog::IsLoaded())
		{
			return ly::content::EffectContentCatalog::FindById(effectId);
		}
		for (const sas::GameplayEffectDefinition* definition :
			GetBuiltinGameplayEffectDefinitions())
		{
			if (definition && definition->effectId == effectId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}
