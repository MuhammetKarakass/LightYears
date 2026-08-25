#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "presentation/ability/frozenThrong/FrozenThrongPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition FrozenThrongHuskBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::FrozenThrong::Actor::Husk::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FrozenThrongHusk;
		definition.lifeTime = 3.f;
		definition.presentationProfileId =
			ly::FrozenThrongPresentationIds::HuskBasic;
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Damage,
				AbilityData::FrozenThrong::DefaultHuskDamage,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Duration,
				3.f,
				0.01f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Radius,
				AbilityData::FrozenThrong::DefaultExplosionRadius,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Range,
				AbilityData::FrozenThrong::DefaultTargetSearchRadius,
				0.f
			},
			sas::GameplayAttribute{
				ly::CollisionAttributeIds::Radius,
				12.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrozenThrong::Actor::Husk::ProjectileSpeed,
				AbilityData::FrozenThrong::DefaultProjectileSpeed,
				0.f
			},
			sas::GameplayAttribute{
				ly::DamageAttributeIds::CryoBuildupPerHit,
				2.f,
				1.f
			},
			sas::GameplayAttribute{
				ly::DamageAttributeIds::CryoBuildupRequired,
				4.f,
				1.f
			},
			sas::GameplayAttribute{
				ly::DamageAttributeIds::CryoBuildupDuration,
				2.5f,
				0.f
			},
			sas::GameplayAttribute{
				ly::DamageAttributeIds::CryoSlowPercent,
				0.25f,
				0.f,
				1.f
			},
			sas::GameplayAttribute{
				ly::DamageAttributeIds::CryoSlowDuration,
				1.5f,
				0.f
			}
		};
		return definition;
	}();

	inline const ly::GameAbilityDefinition FrozenThrong_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FrozenThrong::AbilityId::Basic;
		// This is only the builtin fallback slot. The runtime loadout owns the
		// player's actual binding, so Frozen Throng is not added to the default
		// loadout by declaring this value here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = AbilityData::FrozenThrong::DefaultCooldown;
		definition.duration = AbilityData::FrozenThrong::DefaultDuration;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::FrozenThrong
		};
		definition.displayName = "Frozen Throng";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 150, 225, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Damage,
				AbilityData::FrozenThrong::DefaultHuskDamage,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrozenThrong::Attribute::LuckToHuskScale,
				AbilityData::FrozenThrong::DefaultLuckToHuskScale,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrozenThrong::Attribute::HuskDelay,
				AbilityData::FrozenThrong::DefaultHuskDelay,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrozenThrong::Attribute::TargetSearchRadius,
				AbilityData::FrozenThrong::DefaultTargetSearchRadius,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FrozenThrong::Attribute::DensityRadius,
				AbilityData::FrozenThrong::DefaultDensityRadius,
				0.f
			},
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Radius,
				AbilityData::FrozenThrong::DefaultExplosionRadius,
				0.f
			},
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				AbilityData::FrozenThrong::DefaultAttackPowerScale
			}
		};
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
						-0.30f
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
		definition.damageTags = { ly::DamageTypeSchema::Cryo };
		definition.behaviorType = ly::AbilityBehaviorType::FrozenThrong;
		return definition;
	}();
}
