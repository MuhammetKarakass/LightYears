#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/strikeRun/StrikeRunContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/strikeRun/StrikeRunPresentationIds.h"

namespace AbilityData::StrikeRun
{
	inline const ly::AbilityActorDefinition ActorBombardmentBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Bombardment::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::StrikeRunBombardment;
		// The bombardment actor owns its preview/telegraph/impact timeline. Its
		// lifetime is therefore controlled by the actor rather than a generic
		// projectile lifetime field.
		definition.lifeTime = 0.f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId =
			ly::StrikeRunPresentationIds::BombardmentBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 35.f, 0.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 200.f, 0.01f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 2000.f, 0.01f },
			sas::GameplayAttribute{ Attribute::ImpactCount, 5.f, 1.f },
			sas::GameplayAttribute{ Attribute::ImpactSpan, 1400.f, 1.f },
			sas::GameplayAttribute{ Attribute::TargetingWindow, 3.f, 0.01f },
			sas::GameplayAttribute{ Attribute::FinalTelegraphDuration, 0.8f, 0.01f },
			sas::GameplayAttribute{ Attribute::ImpactDelay, 0.10f, 0.01f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition StrikeRun_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::StrikeRun::AbilityId::Basic;
		// Runtime loadout assignment owns the real input slot. Ability1 only keeps
		// the standalone C++ fallback definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The first press defers this duration. On confirmation the behavior ends
		// immediately so cooldown starts at the second press while the delivery
		// actor continues its own 1.4-second presentation timeline.
		definition.cooldown = 16.f;
		definition.duration = 1.4f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::StrikeRun
		};
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.displayName = "Strike Run";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 120, 190, 255, 255 };
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
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
						4.f
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
		definition.behaviorType = ly::AbilityBehaviorType::StrikeRun;
		return definition;
	}();
}
