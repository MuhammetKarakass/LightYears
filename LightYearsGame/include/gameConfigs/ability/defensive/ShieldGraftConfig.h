#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/shieldGraft/ShieldGraftContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ShieldGraft_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ShieldGraft::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 12.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ShieldGraft
		};
		definition.displayName = "Shield Graft";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupGreen_shield.png";
		definition.accentColor = sf::Color{ 80, 220, 160, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::ShieldGraft::Attribute::ConversionRatio,
				0.40f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ShieldGraft::Attribute::EnergyPowerReference,
				50.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ShieldGraft::Attribute::EnergyPowerScale,
				0.0001f,
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
						-0.20f
					},
					sas::AttributeModifier{
						AbilityData::ShieldGraft::Attribute::ConversionRatio,
						sas::AttributeModifierOperation::Add,
						0.01f
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
		definition.behaviorType = ly::AbilityBehaviorType::ShieldGraft;
		return definition;
	}();
}
