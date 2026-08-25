#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationIds.h"

namespace AbilityData::WingSentinels
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::WingSentinelProjectile;
		definition.lifeTime = 0.60f;
		definition.presentationProfileId = ly::WingSentinelsPresentationIds::Basic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 10.f, 0.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 650.f, 1.f },
			sas::GameplayAttribute{ ly::CollisionAttributeIds::Radius, 5.f, 0.1f },
			sas::GameplayAttribute{ Actor::Projectile::ProjectileSpeed, 1200.f, 1.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition WingSentinels_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::WingSentinels::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 8.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::WingSentinels
		};
		definition.displayName = "Wing Sentinels";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 100, 205, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ AbilityData::WingSentinels::Attribute::DroneCount, 2.f, 2.f, 2.f },
			sas::GameplayAttribute{ AbilityData::WingSentinels::Attribute::SideOffset, 110.f, 1.f },
			sas::GameplayAttribute{ AbilityData::WingSentinels::Attribute::BaseAttackRate, 1.50f, 0.01f },
			sas::GameplayAttribute{ AbilityData::WingSentinels::Attribute::Range, 650.f, 1.f }
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
			ly::AbilityLevelStep{ {
				sas::AttributeModifier{ ly::CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 2.f },
				sas::AttributeModifier{ AbilityData::WingSentinels::Attribute::BaseAttackRate, sas::AttributeModifierOperation::Add, 0.025f },
				sas::AttributeModifier{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -0.25f }
			}, {}, {}, {} }
		);
		definition.levelUpgradeScrapCosts = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::WingSentinels;
		return definition;
	}();
}
