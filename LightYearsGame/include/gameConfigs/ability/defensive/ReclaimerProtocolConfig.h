#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition ReclaimerRepairKitBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::ReclaimerProtocol::Actor::RepairKit::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ReclaimerRepairKit;
		definition.lifeTime = AbilityData::ReclaimerProtocol::DefaultKitLifetime;
		definition.presentationProfileId =
			ly::ReclaimerProtocolPresentationIds::RepairKitBasic;
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Duration,
				AbilityData::ReclaimerProtocol::DefaultKitLifetime,
				0.01f
			},
			sas::GameplayAttribute{
				ly::CollisionAttributeIds::Radius,
				AbilityData::ReclaimerProtocol::DefaultKitCollisionRadius,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ReclaimerProtocol::Actor::RepairKit::HealRatio,
				AbilityData::ReclaimerProtocol::DefaultHealRatio,
				0.f
			}
		};
		return definition;
	}();

	inline const ly::GameAbilityDefinition ReclaimerProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ReclaimerProtocol::AbilityId::Basic;
		// This is only the builtin fallback slot. The runtime loadout owns the
		// player's actual binding, so Reclaimer Protocol is not added to the default
		// loadout by declaring this value here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = AbilityData::ReclaimerProtocol::DefaultCooldown;
		definition.duration = AbilityData::ReclaimerProtocol::DefaultDuration;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ReclaimerProtocol
		};
		definition.displayName = "Reclaimer Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupGreen_shield.png";
		definition.accentColor = sf::Color{ 80, 220, 140, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::ReclaimerProtocol::Attribute::HealRatio,
				AbilityData::ReclaimerProtocol::DefaultHealRatio,
				0.f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						AbilityData::ReclaimerProtocol::Attribute::HealRatio,
						sas::AttributeModifierOperation::Add,
						0.0015f
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
		definition.behaviorType = ly::AbilityBehaviorType::ReclaimerProtocol;
		return definition;
	}();
}