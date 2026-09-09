#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/arcScythes/ArcScythesPresentationIds.h"

namespace AbilityData::ArcScythes
{
	inline const ly::AbilityActorDefinition ActorBeamBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Beam::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ArcScythesBeam;
		definition.lifeTime = 4.1f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId = ly::ArcScythesPresentationIds::BeamBasic;
		definition.attributes = {
			sas::GameplayAttribute{ Attribute::Damage, 5.f, 0.f },
			sas::GameplayAttribute{ Attribute::Range, 700.f, 1.f },
			sas::GameplayAttribute{ Attribute::CombatTickInterval, 0.25f, 0.01f },
			sas::GameplayAttribute{ Attribute::BeamHalfThickness, 22.f, 1.f },
			sas::GameplayAttribute{ Attribute::ElectricStacks, 1.f, 1.f },
			sas::GameplayAttribute{
				Attribute::ElectricDamageTakenMultiplierPerStack, 0.04f, 0.f
			},
			sas::GameplayAttribute{ Attribute::ElectricDuration, 3.f, 0.f },
			sas::GameplayAttribute{ Attribute::ElectricMaxStacks, 4.f, 1.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ArcScythes_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ArcScythes::AbilityId::Basic;
		// Runtime loadouts own the player binding. This is only a valid fallback
		// slot for content loading and does not permanently assign a key.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 4.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ArcScythes
		};
		definition.damageTags = { ly::DamageTypeSchema::Electric };
		definition.displayName = "Arc Scythes";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 230, 255, 255 };
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::EnergyMax,
				sas::AttributeModifierOperation::Add,
				0.06f
			}
		};
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::ArcScythes::Actor::Beam::BasicDefinitionId,
					sas::AbilitySpawnPolicy::AtOwner
				},
				0.f,
				1
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
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ArcScythes;
		return definition;
	}();
}
