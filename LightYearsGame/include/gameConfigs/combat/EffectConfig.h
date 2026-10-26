#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/combat/EffectStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/damage/ArmorBreak.h"
#include "gameplay/weapon/ShotDeviation.h"
#include "gameConfigs/ability/utility/GravityAnomalyConfig.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/effects/EffectBehaviorKeys.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h"
#include "presentation/effects/shield/ShieldVisualIds.h"

namespace EffectData
{
	// Effect behavior selectors belong to the content/registry boundary, not to
	// gameplay schemas or ability contracts.
	inline const sas::GameplayEffectBehaviorKey& BarrierBehaviorKey =
		vega::EffectBehaviorKeys::Barrier;
	inline const sas::GameplayEffectBehaviorKey& DamageIgniteBehaviorKey =
		vega::EffectBehaviorKeys::DamageIgnite;
	inline const sas::GameplayEffectBehaviorKey& DamageElectricBehaviorKey =
		vega::EffectBehaviorKeys::DamageElectric;
	inline const sas::GameplayEffectBehaviorKey& GravityAnomalyBehaviorKey =
		vega::EffectBehaviorKeys::GravityAnomaly;
	inline const sas::GameplayEffectBehaviorKey& DirectionalBarrierBehaviorKey =
		vega::EffectBehaviorKeys::DirectionalBarrier;
	inline const sas::GameplayEffectBehaviorKey& DamageReductionBehaviorKey =
		vega::EffectBehaviorKeys::DamageReduction;

	// JSON fallback/test catalog only. Shipped runtime policy comes from
	// effects.json; source-owned magnitudes and durations come from the weapon,
	// ability, enemy or reward that creates GameplayEffectSpec.
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
		vega::ShieldVisualIds::Basic,
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
			sas::AttributeModifier{ vega::OwnerAttributeIds::MoveSpeedVertical, sas::AttributeModifierOperation::Add, 0.25f, 0 },
			sas::AttributeModifier{ vega::OwnerAttributeIds::MoveSpeedHorizontal, sas::AttributeModifierOperation::Add, 0.2f, 0 }
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
		vega::DamageStatusEffectIds::IgniteEffectId,
		DamageIgniteBehaviorKey,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		0.f,
		1,
		{ vega::DamageStatusSchema::Ignite },
		{},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Damage.OverTime",
		"",
		"",
		sas::GameplayEffectStackLifetimePolicy::DecayAfterDuration,
		1.f
	};

	inline const sas::GameplayEffectDefinition CryoSlowEffect{
		vega::DamageStatusEffectIds::CryoSlowedEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		0.f,
		1,
		{ vega::DamageStatusSchema::CryoSlowed },
		{},
		{},
		"",
		{},
		{ MovementEffectSchema::SlowImmunityGrantedTag },
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Movement.Slow",
		"Movement.Slow",
		"",
		sas::GameplayEffectStackLifetimePolicy::None,
		0.f
	};

	inline const sas::GameplayEffectDefinition ElectricEffect{
		vega::DamageStatusEffectIds::ElectricEffectId,
		DamageElectricBehaviorKey,
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		0.f,
		1,
		{ vega::DamageStatusSchema::Electric },
		{},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Damage.Electric",
		"",
		"",
		sas::GameplayEffectStackLifetimePolicy::None,
		0.f
	};

	inline const sas::GameplayEffectDefinition KineticEffect{
		vega::DamageStatusEffectIds::KineticEffectId,
		{},
		sas::GameplayEffectDurationPolicy::Duration,
		sas::GameplayEffectStackingPolicy::Stack,
		0.f,
		1,
		{ vega::DamageStatusSchema::Kinetic },
		{},
		{},
		"",
		{},
		{},
		false,
		false,
		sas::GameplayEffectDisposition::Harmful,
		true,
		"Damage.Kinetic",
		"",
		"",
		sas::GameplayEffectStackLifetimePolicy::None,
		0.f
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
			vega::GameplayTags::State::Effect::GravityAnomaly::Inside
		};
		definition.applicationBlockedTags = {
			MovementEffectSchema::SlowImmunityGrantedTag
		};
		definition.modifiers = {
			sas::AttributeModifier{
				vega::OwnerAttributeIds::MovementSlow,
				sas::AttributeModifierOperation::Add,
				// GravityAnomalyFieldActor resolves this value from the JSON-loaded
				// actor attributes before applying the effect spec.
				0.f
			}
		};
		definition.activeVisualId = vega::GravityAnomalyEffectVisualIds::Inside;
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
				vega::OwnerAttributeIds::MovementSlow,
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
			vega::GameplayTags::State::Effect::OverdriveCore::AttackSpeedBoost
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
			vega::GameplayTags::State::Effect::Offense::ExecutionDrive::AttackPower
		};
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Offense.ExecutionDrive.AttackPower";
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
			vega::GameplayTags::State::Effect::Control::Stunned
		};
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.cleanseable = true;
		definition.category = "Control.Stun";
		definition.immunityCategory = "Control.Stun";
		definition.sourceParameterized = true;
		return definition;
	}();

	inline const sas::GameplayEffectDefinition CryoFrozenEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = vega::DamageStatusEffectIds::CryoFrozenEffectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.grantedTags = { vega::DamageStatusSchema::CryoFrozen, vega::GameplayTags::State::Effect::Control::Stunned };
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.cleanseable = true;
		definition.category = "Control.Stun";
		definition.immunityCategory = "Control.Stun";
		definition.sourceParameterized = true;
		return definition;
	}();
	inline const sas::GameplayEffectDefinition CryoFreezeCooldownEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = vega::DamageStatusEffectIds::CryoFreezeCooldownEffectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.category = "Runtime.CryoFreezeCooldown";
		definition.sourceParameterized = true;
		return definition;
	}();
	inline const sas::GameplayEffectDefinition KineticArmorRendEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = vega::DamageStatusEffectIds::KineticArmorRendEffectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.category = "Runtime.KineticArmorRend";
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.sourceParameterized = true;
		return definition;
	}();
	// Shared timed debuffs (ShotDeviation.h, ArmorBreak.h); their values travel in the applied spec.
	inline const sas::GameplayEffectDefinition ShotDeviationEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = vega::ShotDeviationIds::EffectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.category = "Debuff.ShotDeviation";
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.sourceParameterized = true;
		return definition;
	}();
	inline const sas::GameplayEffectDefinition ArmorBreakEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = vega::ArmorBreakIds::EffectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.category = "Debuff.ArmorBreak";
		definition.disposition = sas::GameplayEffectDisposition::Harmful;
		definition.sourceParameterized = true;
		return definition;
	}();
	inline const sas::GameplayEffectDefinition DirectionalBarrierActiveEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::DirectionalBarrier::Effect::ActiveEffectId;
		definition.behaviorKey = DirectionalBarrierBehaviorKey;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.grantedTags = {
			AbilityData::DirectionalBarrier::State::Active
		};
		definition.sourceScopedApplication = true;
		definition.sourceParameterized = true;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.DirectionalBarrier";
		return definition;
	}();

	inline const sas::GameplayEffectDefinition CryostasisIceShellEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::Cryostasis::Effect::IceShellId;
		definition.behaviorKey = vega::EffectBehaviorKeys::CryostasisIceShell;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.grantedTags = { vega::GameplayTags::State::Effect::Defense::Cryostasis::IceShell };
		definition.sourceScopedApplication = true;
		definition.sourceParameterized = true;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.Cryostasis.IceShell";
		definition.grantedImmunityCategory = "Control";
		return definition;
	}();

	inline const sas::GameplayEffectDefinition IroncladProtocolDamageReductionEffect = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = AbilityData::IroncladProtocol::Effect::DamageReductionId;
		definition.behaviorKey = DamageReductionBehaviorKey;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.sourceScopedApplication = true;
		definition.sourceParameterized = true;
		definition.disposition = sas::GameplayEffectDisposition::Beneficial;
		definition.category = "Defense.DamageReduction";
		return definition;
	}();

	inline const vega::List<const sas::GameplayEffectDefinition*>&
	GetBuiltinGameplayEffectDefinitions()
	{
		static const vega::List<const sas::GameplayEffectDefinition*> definitions{
			&BasicBarrierEffect,
			&BarrierBreakThrustBoostEffect,
			&IgniteEffect,
			&CryoSlowEffect,
			&ElectricEffect,
			&KineticEffect,
			&GravityAnomalyInsideEffect,
			&MovementSlowEffect,
			&MovementSlowImmunityEffect,
			&OverdriveCoreAttackSpeedBoostEffect,
			&ExecutionDriveAttackPowerEffect,
			&NullPulseStunEffect,
			&CryoFrozenEffect,
			&CryoFreezeCooldownEffect,
			&KineticArmorRendEffect,
			&ShotDeviationEffect,
			&ArmorBreakEffect,
			&DirectionalBarrierActiveEffect,
			&CryostasisIceShellEffect,
			&IroncladProtocolDamageReductionEffect
		};
		return definitions;
	}

	inline const vega::List<const sas::GameplayEffectDefinition*>&
	GetShippedGameplayEffectDefinitions()
	{
		if (vega::content::EffectContentCatalog::IsLoaded())
		{
			return vega::content::EffectContentCatalog::GetDefinitions();
		}
		return GetBuiltinGameplayEffectDefinitions();
	}

	inline const sas::GameplayEffectDefinition* FindGameplayEffectDefinition(const std::string& effectId)
	{
		if (vega::content::EffectContentCatalog::IsLoaded())
		{
			return vega::content::EffectContentCatalog::FindById(effectId);
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
