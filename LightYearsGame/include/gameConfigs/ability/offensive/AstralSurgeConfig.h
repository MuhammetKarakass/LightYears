#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"

#include <cstddef>

namespace AbilityData::AstralSurge
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::AstralSurgeProjectile;
		definition.presentationProfileId = ly::AstralSurgePresentationIds::ProjectileBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 70.f, 0.f },
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
		definition.cooldown = 14.f;
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
				0.60f
			}
		};
		definition.levelProgression.reserve(14);
		for (std::size_t stepIndex = 0; stepIndex < 14; ++stepIndex)
		{
			const float cooldownDelta = stepIndex < 4 ? -0.525f :
				(stepIndex < 8 ? -0.425f : (stepIndex < 12 ? -0.325f : -0.225f));
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						8.f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						cooldownDelta
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
						0.05f
					}
				}
			});
		}
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
