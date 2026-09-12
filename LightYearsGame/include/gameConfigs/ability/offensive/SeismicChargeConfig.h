#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/seismicCharge/SeismicChargePresentationIds.h"

namespace AbilityData::SeismicCharge
{
	inline const ly::AbilityActorDefinition ActorBombBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Bomb::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::SeismicChargeBomb;
		definition.lifeTime = 5.6f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId = ly::SeismicChargePresentationIds::BombBasic;
		definition.attributes = {
			sas::GameplayAttribute{ Attribute::Damage, 100.f, 0.f },
			sas::GameplayAttribute{ Attribute::MaximumRadius, 1200.f, 1.f },
			sas::GameplayAttribute{ Attribute::DropOffset, 250.f, 0.f },
			sas::GameplayAttribute{ Attribute::DeploymentDuration, 0.50f, 0.01f },
			sas::GameplayAttribute{ Attribute::FuseDuration, 3.f, 0.01f },
			sas::GameplayAttribute{ Attribute::ShockwaveDuration, 2.f, 0.01f },
			sas::GameplayAttribute{ Attribute::ShockwaveThickness, 120.f, 1.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition SeismicCharge_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::SeismicCharge::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 21.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::SeismicCharge
		};
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.displayName = "Seismic Charge";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 85, 190, 255, 255 };
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.80f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{ ly::CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 7.f },
					sas::AttributeModifier{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -0.30f }
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::SeismicCharge;
		return definition;
	}();
}
