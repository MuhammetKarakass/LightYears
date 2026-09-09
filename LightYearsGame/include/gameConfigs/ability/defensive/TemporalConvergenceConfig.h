#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationIds.h"

namespace AbilityData::TemporalConvergence
{
	inline const ly::AbilityActorDefinition ActorFieldBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::TemporalConvergenceField;
		// 2.0 dormant + 1.6 maximum travel + 2.0 field, with a small frame buffer.
		definition.lifeTime = 5.8f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId =
			ly::TemporalConvergencePresentationIds::FieldBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, 5.8f, 0.01f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition TemporalConvergence_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TemporalConvergence::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 13.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::TemporalConvergence
		};
		definition.displayName = "Temporal Convergence";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 125, 195, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 1600.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 400.f, 1.f },
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::BaseShield, 120.f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::InitialDelay, 2.f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::TravelSpeed, 1000.f, 1.f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::FieldDuration, 2.f, 0.01f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::SlowFraction, 0.40f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::StunDuration, 2.f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::OvershieldHoldDuration, 4.f, 0.f
			},
			// At this rate duration is calculated from the excess that remains after
			// hold: 120 excess / 100 per second = 1.2 seconds of decay.
			sas::GameplayAttribute{
				AbilityData::TemporalConvergence::Attribute::OvershieldDecayPerSecond,
				100.f,
				0.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				AbilityData::TemporalConvergence::Attribute::BaseShield,
				ly::OwnerAttributeIds::EnergyMax,
				sas::AttributeModifierOperation::Add,
				0.60f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						AbilityData::TemporalConvergence::Attribute::BaseShield,
						sas::AttributeModifierOperation::Add,
						10.f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.30f
					}
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::TemporalConvergence;
		return definition;
	}();
}
