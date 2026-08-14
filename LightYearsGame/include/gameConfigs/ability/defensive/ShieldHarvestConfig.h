#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestContracts.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ShieldHarvest_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ShieldHarvest::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 1.5f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ShieldHarvest
		};
		definition.displayName = "Shield Harvest";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 95, 220, 255, 220 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 700.f, 1.f },
			sas::GameplayAttribute{
				AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
				40.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ShieldHarvest::Attribute::OvershieldHoldDuration,
				5.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ShieldHarvest::Attribute::OvershieldDecayPerSecond,
				100.f,
				0.f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.25f
					},
					sas::AttributeModifier{
						AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
						sas::AttributeModifierOperation::Add,
						5.f
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
		definition.behaviorType = ly::AbilityBehaviorType::ShieldHarvest;
		return definition;
	}();
}
