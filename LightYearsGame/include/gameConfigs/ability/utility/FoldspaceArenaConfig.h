#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationIds.h"

namespace AbilityData::FoldspaceArena
{
	inline const ly::AbilityActorDefinition ActorArenaBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Arena::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FoldspaceArena;
		// Runtime duration is resolved from EnergyPower. This is only a safe ceiling
		// so actor cleanup still succeeds if future content is misconfigured.
		definition.lifeTime = 10.f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId = ly::FoldspaceArenaPresentationIds::ArenaBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, 10.f, 0.01f },
			sas::GameplayAttribute{ Actor::Arena::Width, 1500.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::Height, 1000.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::ProjectileSpeed, 1200.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::CornerRadius, 250.f, 0.f },
			sas::GameplayAttribute{ Actor::Arena::WrapInwardOffset, 20.f, 0.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition FoldspaceArena_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FoldspaceArena::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The arena ends itself at its resolved 6–8 second lifetime. This is the
		// outer lifecycle ceiling required by the common duration ability runtime.
		definition.duration = 10.f;
		definition.cooldown = 17.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::FoldspaceArena
		};
		definition.damageTags = { ly::DamageTypeSchema::Photonic };
		definition.displayName = "Foldspace Arena";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 145, 215, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 300.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 15.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::MinimumArenaDuration, 1.f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::BaseArenaDuration, 6.f, 0.01f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::EnergyPowerDurationReference,
				50.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::EnergyPowerDurationPerPoint,
				0.002f,
				0.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyPower,
				sas::AttributeModifierOperation::Add,
				0.08f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						2.f
					},
					sas::AttributeModifier{
						AbilityData::FoldspaceArena::Attribute::BaseArenaDuration,
						sas::AttributeModifierOperation::Add,
						0.10f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.25f
					}
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::FoldspaceArena;
		return definition;
	}();
}
