#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition FrostMaelstromFieldBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::FrostMaelstrom::Actor::Field::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FrostMaelstromField;
		definition.presentationProfileId =
			ly::FrostMaelstromPresentationIds::FieldBasic;
		return definition;
	}();

	inline const ly::GameAbilityDefinition FrostMaelstrom_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FrostMaelstrom::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = AbilityData::FrostMaelstrom::DefaultCooldown;
		definition.duration = AbilityData::FrostMaelstrom::DefaultDuration;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Control,
			ly::GameplayTags::Ability::Family::FrostMaelstrom
		};
		definition.displayName = "Frost Maelstrom";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 115, 220, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::MinimumRadius,
				AbilityData::FrostMaelstrom::DefaultMinimumRadius,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::MaximumRadius,
				AbilityData::FrostMaelstrom::DefaultMaximumRadius,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::MinimumMovementSpeed,
				AbilityData::FrostMaelstrom::DefaultMinimumMovementSpeed,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::MaximumMovementSpeed,
				AbilityData::FrostMaelstrom::DefaultMaximumMovementSpeed,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::TickInterval,
				AbilityData::FrostMaelstrom::DefaultTickInterval,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::CryoStacksPerTick,
				AbilityData::FrostMaelstrom::DefaultCryoStacksPerTick,
				1.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::OrbitalAngularSpeed,
				AbilityData::FrostMaelstrom::DefaultOrbitalAngularSpeed,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::InwardForce,
				AbilityData::FrostMaelstrom::DefaultInwardForce,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio,
				AbilityData::FrostMaelstrom::DefaultOrbitalRadiusRatio,
				0.01f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::EnergyMaxReference,
				AbilityData::FrostMaelstrom::DefaultEnergyMaxReference,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::EnergyMaxDamageScale,
				AbilityData::FrostMaelstrom::DefaultEnergyMaxDamageScale,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrostMaelstrom::Attribute::EnergyMaxRadiusScale,
				AbilityData::FrostMaelstrom::DefaultEnergyMaxRadiusScale,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Damage,
				AbilityData::FrostMaelstrom::DefaultTickDamage,
				0.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.03f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.20f
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
		definition.behaviorType = ly::AbilityBehaviorType::FrostMaelstrom;
		return definition;
	}();
}
