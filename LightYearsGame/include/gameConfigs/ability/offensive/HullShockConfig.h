#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition HullShock_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::HullShock::AbilityId::Basic;
		// Ability1 is mapped to Q. Hull Shock replaces the previous default
		// ability in that input slot; Shield Harvest keeps the defensive F slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::WhileHeld;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::WhileInputHeld;
		definition.cooldown = 12.f;
		definition.duration = 2.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::HullShock
		};
		definition.displayName = "Hull Shock";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 75, 244, 255, 235 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 600.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 30.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::FullRadiusDuration,
				1.25f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::MinimumChargeRadius,
				300.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::ElectricChargeThreshold,
				1.80f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::MinimumChargeDamageMultiplier,
				0.25f,
				0.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::ElectricDamageTakenMultiplierPerStack,
				0.04f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::ElectricDuration,
				3.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::HullShock::Attribute::ElectricMaxStacks,
				4.f,
				1.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::MaxHealth,
				sas::AttributeModifierOperation::Add,
				0.30f
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
		definition.damageTags = { ly::DamageTypeSchema::Electric };
		definition.behaviorType = ly::AbilityBehaviorType::HullShock;
		return definition;
	}();
}
