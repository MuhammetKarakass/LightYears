#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/solarBombardment/SolarBombardmentPresentationIds.h"

namespace AbilityData::SolarBombardment
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::SolarBombardmentProjectile;
		definition.lifeTime = 3.f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId =
			ly::SolarBombardmentPresentationIds::ProjectileBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 45.f, 0.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 350.f, 0.01f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 2000.f, 0.01f },
			sas::GameplayAttribute{ Attribute::InnerRadius, 150.f, 0.01f },
			sas::GameplayAttribute{ Attribute::InnerDamageMultiplier, 2.f, 1.f },
			sas::GameplayAttribute{ Attribute::InnerIgniteStacks, 4.f, 1.f },
			sas::GameplayAttribute{ Attribute::OuterIgniteStacks, 2.f, 1.f },
			sas::GameplayAttribute{ Attribute::MinTravelTime, 0.8f, 0.01f },
			sas::GameplayAttribute{ Attribute::MaxTravelTime, 1.6f, 0.01f },
			sas::GameplayAttribute{ ly::DamageAttributeIds::IgniteStacks, 4.f, 0.f },
			sas::GameplayAttribute{ ly::DamageAttributeIds::BurnDamagePerSecond, 1.f, 0.f },
			sas::GameplayAttribute{ ly::DamageAttributeIds::BurnDuration, 3.f, 0.f },
			sas::GameplayAttribute{ ly::DamageAttributeIds::BurnMaxStacks, 4.f, 1.f },
			sas::GameplayAttribute{ ly::CollisionAttributeIds::Radius, 1.f, 0.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition SolarBombardment_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::SolarBombardment::AbilityId::Basic;
		// Runtime loadout assignment owns the real input slot. This fallback slot
		// only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 14.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::SolarBombardment
		};
		definition.damageTags = { ly::DamageTypeSchema::Thermal };
		definition.displayName = "Solar Bombardment";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
		definition.accentColor = sf::Color{ 255, 125, 35, 255 };
		// Activation is implemented by the family behavior because it must pass
		// the cursor-clamped target location into the projectile actor.
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.80f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						6.f
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
		definition.behaviorType = ly::AbilityBehaviorType::SolarBombardment;
		return definition;
	}();
}
