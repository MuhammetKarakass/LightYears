#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/scorchDrive/ScorchDriveContracts.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/scorchDrive/ScorchDrivePresentationIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ScorchDrive
{
	inline const ly::AbilityActorDefinition ActorFireSegmentBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::FireSegment::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ScorchDriveFireSegment;
		definition.lifeTime = 5.f;
		definition.presentationProfileId =
			ly::ScorchDrivePresentationIds::FireSegmentBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::AreaAttributeIds::Width, 70.f, 0.1f },
			sas::GameplayAttribute{ ly::AreaAttributeIds::Length, 80.f, 0.1f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ScorchDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ScorchDrive::AbilityId::Basic;
		// The runtime loadout decides the actual input slot. This transitional
		// value only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 13.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ScorchDrive
		};
		definition.damageTags = {
			ly::DamageTypeSchema::Thermal
		};
		definition.displayName = "Scorch Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Effects/fire01.png";
		definition.accentColor = sf::Color{ 255, 95, 25, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 8.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::SegmentSpawnDistance,
				60.f,
				0.1f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::BaseSegmentLifetime,
				5.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::FireTickInterval,
				0.25f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::BurnThresholdTicks,
				4.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::BurnDuration,
				3.f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::BurnTickInterval,
				0.5f,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::BurnDamageRatio,
				0.5f,
				0.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::ReferenceMaxHealth,
				100.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ScorchDrive::Attribute::MaxHealthLifetimeScale,
				0.01f,
				0.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
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
		definition.behaviorType = ly::AbilityBehaviorType::ScorchDrive;
		return definition;
	}();
}
