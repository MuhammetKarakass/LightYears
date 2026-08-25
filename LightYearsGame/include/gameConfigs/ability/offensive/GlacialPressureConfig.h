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
		definition.cooldown = 12.f;
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
				12.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::CollisionDamage,
				45.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::EnergyMaxInitialScale,
				0.05f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::GlacialPressure::Attribute::EnergyMaxCollisionScale,
				0.25f,
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
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						AbilityData::GlacialPressure::Attribute::InitialDamage,
						sas::AttributeModifierOperation::Add,
						1.f
					},
					sas::AttributeModifier{
						AbilityData::GlacialPressure::Attribute::CollisionDamage,
						sas::AttributeModifierOperation::Add,
						4.f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.25f
					}
				},
				{},
				{},
				{}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.damageTags = { ly::DamageTypeSchema::Cryo };
		definition.behaviorType = ly::AbilityBehaviorType::GlacialPressure;
		return definition;
	}();
}
