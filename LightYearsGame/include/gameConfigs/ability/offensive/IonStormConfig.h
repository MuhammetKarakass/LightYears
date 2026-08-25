#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/ionStorm/IonStormPresentationIds.h"

namespace AbilityData::IonStorm
{
	inline const sas::GameplayAttributeList FieldAttributes{
		sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, 4.f, 0.01f },
		sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 335.f, 0.01f },
		sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 6.f, 0.f },
		sas::GameplayAttribute{ Attribute::TickInterval, 0.25f, 0.01f },
		sas::GameplayAttribute{ Attribute::InnerCoreRadius, 250.f, 0.01f },
		sas::GameplayAttribute{ Attribute::OuterMinRadius, 250.f, 0.01f },
		sas::GameplayAttribute{ Attribute::OuterMaxRadius, 335.f, 0.01f },
		sas::GameplayAttribute{ Attribute::BoundaryPointCount, 20.f, 3.f }
	};

	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::IonStormProjectile;
		definition.spawnDistance = 42.f;
		definition.presentationProfileId =
			ly::IonStormPresentationIds::ProjectileBasic;
		definition.attributes = FieldAttributes;
		definition.attributes.push_back(sas::GameplayAttribute{
			ly::CommonAttributeIds::Range,
			900.f,
			0.01f
		});
		definition.attributes.push_back(sas::GameplayAttribute{
			Attribute::ProjectileSpeed,
			2000.f,
			0.01f
		});
		return definition;
	}();

	inline const ly::AbilityActorDefinition ActorFieldBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::IonStormField;
		definition.lifeTime = 4.f;
		definition.presentationProfileId =
			ly::IonStormPresentationIds::FieldBasic;
		definition.attributes = FieldAttributes;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition IonStorm_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::IonStorm::AbilityId::Basic;
		// Runtime loadout assignment owns the real input slot. This fallback slot
		// only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 10.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::IonStorm
		};
		definition.damageTags = { ly::DamageTypeSchema::Electric };
		definition.displayName = "Ion Storm";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 190, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::IonStorm::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.12f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						1.f
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
		definition.behaviorType = ly::AbilityBehaviorType::IonStorm;
		return definition;
	}();
}
