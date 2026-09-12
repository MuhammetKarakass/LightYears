#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"

namespace AbilityData::AstralSurge
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::AstralSurgeProjectile;
		definition.presentationProfileId = ly::AstralSurgePresentationIds::ProjectileBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 50.f, 0.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::PierceDamageLoss, 0.05f, 0.f, 0.99f },
			sas::GameplayAttribute{ ly::AreaAttributeIds::Width, 280.f, 0.1f },
			sas::GameplayAttribute{ Actor::Projectile::ProjectileSpeed, 1000.f, 0.01f },
			sas::GameplayAttribute{ Actor::Projectile::MinimumDamageMultiplier, 0.40f, 0.f, 1.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition AstralSurge_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::AstralSurge::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 16.f;
		definition.duration = 1.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::AstralSurge
		};
		definition.displayName = "Astral Surge";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 175, 105, 255, 255 };
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.45f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						5.f
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
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::AstralSurge;
		return definition;
	}();
}
