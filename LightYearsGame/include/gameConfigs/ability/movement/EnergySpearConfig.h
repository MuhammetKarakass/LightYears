#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition EnergySpear_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EnergySpear::AbilityId::Basic;
		// Q is the shipped default binding. Runtime loadout code may still equip
		// Energy Spear into any available Ability1-Ability4 slot later.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::WhileHeld;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::WhileInputHeld;
		definition.cooldown = 8.f;
		definition.duration = 1.5f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Movement,
			ly::GameplayTags::Ability::Family::EnergySpear
		};
		definition.displayName = "Energy Spear";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 190, 250, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ AbilityData::EnergySpear::Attribute::Damage, 10.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::MaximumDistance,
				600.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::CollisionRadius,
				18.f,
				0.1f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::MinimumDistance,
				200.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold,
				0.90f,
				0.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::ChargeDamageMultiplierAtFull,
				2.50f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::DistanceDamageMultiplierAtEndpoint,
				2.0f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::EnergyMaxReference,
				50.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::EnergyMaxDistanceScale,
				2.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EnergySpear::Attribute::TravelSpeed,
				2400.f,
				1.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				AbilityData::EnergySpear::Attribute::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				1.0f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						2.f
					},
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
		definition.damageTags = { ly::GameplayTags::Damage::Type::Energy };
		definition.attachmentCapabilities = {
			ly::GameplayTags::Attachment::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::EnergySpear;
		return definition;
	}();
}
