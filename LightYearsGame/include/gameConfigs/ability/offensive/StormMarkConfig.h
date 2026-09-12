#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/stormMark/StormMarkContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition StormMark_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::StormMark::AbilityId::Basic;
		// Ability1 is only the standalone fallback slot. The runtime loadout
		// remains the owner of the player's actual binding.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 9.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::StormMark
		};
		definition.displayName = "Storm Mark";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 65, 190, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Damage,
				25.f,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Range,
				600.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::BaseTargetCount,
				4.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::FocusDuration,
				0.30f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::StrikeDuration,
				0.20f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::LuckPerExtraTarget,
				50.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::ElectricStacks,
				1.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::ElectricDamageTakenMultiplierPerStack,
				0.04f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::ElectricDuration,
				3.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::StormMark::Attribute::ElectricMaxStacks,
				4.f,
				1.f
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Electric };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Area
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.25f
			}
		};
		// Behavior owns focus, target snapshot and delayed strikes. No generic
		// action is declared because it would execute before the focus completes.
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						4.f
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
		definition.behaviorType = ly::AbilityBehaviorType::StormMark;
		return definition;
	}();
}
