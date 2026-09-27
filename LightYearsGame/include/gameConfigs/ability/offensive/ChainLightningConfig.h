#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/chainLightning/ChainLightningContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <algorithm>

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ChainLightning_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ChainLightning::AbilityId::Basic;
		// The runtime loadout manager may bind this ability to any Ability1-4
		// slot. Ability1 is only a valid standalone content fallback and does not
		// assign a permanent player key.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 11.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ChainLightning
		};
		definition.displayName = "Chain Lightning";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 230, 255, 255 };
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.50f
			}
		};
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Damage,
				40.f,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Range,
				700.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ChainLightning::Attribute::BounceRange,
				350.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ChainLightning::Attribute::BaseBounceCount,
				5.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::ChainLightning::Attribute::LinkTravelTime,
				0.22f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ChainLightning::Attribute::ElectricStacks,
				1.f,
				1.f
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Electric };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		// The chain traversal owns targeting and delayed impact. There must be no
		// generic action here that could apply damage immediately on activation.
		definition.levelProgression.reserve(14);
		float resolvedCooldown = definition.cooldown;
		float cooldownReduction = 0.175f + 0.025f * definition.cooldown;
		const float minimumCooldown = definition.cooldown * 0.20f;
		for (int level = 0; level < 14; ++level)
		{
			const float cooldownDelta = -std::min(
				cooldownReduction,
				resolvedCooldown - minimumCooldown
			);
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					{ ly::CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 5.f },
					{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, cooldownDelta }
				},
				{},
				{},
				{},
				{
					{ ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::EnergyPower,
						sas::AttributeModifierOperation::Add, 0.05f }
				}
			});
			resolvedCooldown += cooldownDelta;
			if ((level + 1) % 4 == 0)
			{
				cooldownReduction = cooldownReduction >= 0.20f
					? cooldownReduction - 0.10f
					: cooldownReduction * 0.80f;
			}
		}
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ChainLightning;
		return definition;
	}();
}
