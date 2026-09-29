#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/emberSwarm/EmberSwarmContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition EmberSwarm_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EmberSwarm::AbilityId::Basic;
		// Default runtime ability slot mapping has one owner only: DefaultAbilityLoadout.cpp.
		// Fallback config specifies Ability1 so it is a valid loadout ability when granted.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 6.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::EmberSwarm
		};
		definition.displayName = "Ember Swarm";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 120, 30, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 6.f, 0.f }
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.08f
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Thermal };
		ly::SetRepeatingAbilityLevelStep(definition, ly::AbilityLevelStep{
			{
				sas::AttributeModifier{
					ly::CommonAttributeIds::Damage,
					sas::AttributeModifierOperation::Add,
					2.f
				}
			},
			{},
			{},
			{},
			{
				sas::AttributeScalingRule{
					ly::CommonAttributeIds::Damage,
					ly::OwnerAttributeIds::EnergyPower,
					sas::AttributeModifierOperation::Add,
					0.01f
				}
			}
		});
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::EmberSwarm;
		return definition;
	}();
}
