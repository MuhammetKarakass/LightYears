#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition GlacialPressure_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::GlacialPressure::AbilityId::Basic;
		// The shipped default loadout explicitly binds Glacial Pressure to R
		// (Ability4). Acquired abilities may still be rebound at runtime.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 10.f;
		// The behavior treats definition.duration as the one-second focus period
		// and extends the actual active lifetime by the controlled push duration.
		definition.duration = 1.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::GlacialPressure
		};
		definition.displayName = "Glacial Pressure";
		// Keep the icon reference inside the shipped asset set. The ability's
		// gameplay identity is independent from this presentation texture.
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 155, 235, 255, 235 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::Range,
				AbilityData::GlacialPressure::DefaultConeLength,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::InitialDamage,
				30.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::CollisionDamage,
				60.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale,
				0.20f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale,
				0.20f,
				0.f,
				0.80f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::MaxHealthCollisionScale,
				0.40f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::MaxHealthReference,
				100.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::MaxHealthPushScale,
				0.0005f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::PushDistance,
				AbilityData::GlacialPressure::DefaultPushDistance,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::PushDuration,
				AbilityData::GlacialPressure::DefaultImpulseWindowDuration,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::ConeHalfAngleDegrees,
				AbilityData::GlacialPressure::DefaultConeHalfAngleDegrees,
				1.f,
				89.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::SegmentCount,
				5.f,
				1.f,
				5.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::SegmentOneExtraStun,
				AbilityData::GlacialPressure::DefaultCollisionStunDuration,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::PushStunDuration,
				AbilityData::GlacialPressure::DefaultPushStunDuration,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::CollisionStunDuration,
				AbilityData::GlacialPressure::DefaultCollisionStunDuration,
				0.f
			}
		};
		for (int targetLevel = 2; targetLevel <= 25; ++targetLevel)
		{
			const std::size_t stepIndex = static_cast<std::size_t>(targetLevel - 2);
			const float cooldownReduction = ly::GetGlobalAbilityCooldownStepReduction(
				definition.cooldown,
				stepIndex
			);
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					{ AbilityData::GlacialPressure::Attribute::InitialDamage, sas::AttributeModifierOperation::Add, 5.f },
					{ AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale, sas::AttributeModifierOperation::Add, 0.02f },
					{ AbilityData::GlacialPressure::Attribute::CollisionDamage, sas::AttributeModifierOperation::Add, 10.f },
					{ AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale, sas::AttributeModifierOperation::Add, 0.04f },
					{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -cooldownReduction }
				},
				{},
				{},
				{},
				{}
			});
		}
		definition.levelUpgradeScrapCosts.assign(24, 60);
		definition.damageTags = { ly::DamageTypeSchema::Cryo };
		definition.behaviorType = ly::AbilityBehaviorType::GlacialPressure;
		return definition;
	}();
}
