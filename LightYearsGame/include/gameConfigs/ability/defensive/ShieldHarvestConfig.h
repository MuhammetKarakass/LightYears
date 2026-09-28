#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestContracts.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <algorithm>

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
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 700.f, 1.f },
			sas::GameplayAttribute{
				AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
				20.f,
				0.f
			},
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
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
		definition.scalingRules = {
			{ AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
				ly::OwnerAttributeIds::EnergyPower, sas::AttributeModifierOperation::Add, 0.10f },
			{ ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower, sas::AttributeModifierOperation::Add, 0.05f }
		};
		float cooldown = definition.cooldown;
		float reduction = 0.175f + 0.025f * cooldown;
		for (int index = 0; index < 14; ++index)
		{
			if (index && index % 4 == 0)
			{
				reduction = reduction >= 0.20f ? reduction - 0.10f : reduction * 0.80f;
			}
			const float nextCooldown = std::max(definition.cooldown * 0.20f, cooldown - reduction);
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					{ AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
						sas::AttributeModifierOperation::Add, 5.f },
					{ ly::CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 2.f },
					{ ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add, nextCooldown - cooldown }
				}, {}, {}, {},
				{ { AbilityData::ShieldHarvest::Attribute::ShieldPerEnemy,
					ly::OwnerAttributeIds::EnergyPower, sas::AttributeModifierOperation::Add, 0.01f } }
			});
			cooldown = nextCooldown;
		}
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ShieldHarvest;
		return definition;
	}();
}
