#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationIds.h"

namespace AbilityData::LanceDrive
{
	// This local definition is the C++ owner of the actor type and its typed
	// presentation profile; shipped JSON supplies only balance values.
	inline const ly::AbilityActorDefinition ActorLanceBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Lance::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::LanceDrive;
		definition.presentationProfileId = ly::LanceDrivePresentationIds::LanceBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition LanceDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::LanceDrive::AbilityId::Basic;
		// This is only the transitional catalog default; runtime key assignment
		// remains owned by the loadout system and is intentionally not changed here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.cooldown = 15.f;
		definition.duration = 6.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::LanceDrive
		};
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.displayName = "Lance Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 255, 174, 76, 255 };
		definition.attributes = {
			{ AbilityData::LanceDrive::Attribute::BaseDamage, 10.f, 0.f },
			{ AbilityData::LanceDrive::Attribute::SpeedDamageConversion, 0.20f, 0.f },
			{ AbilityData::LanceDrive::Attribute::EnergyMaxReference, 50.f, 0.f },
			{ AbilityData::LanceDrive::Attribute::EnergyMaxConversionPerPoint, 0.0001f, 0.f },
			{ AbilityData::LanceDrive::Attribute::TopSpeedBonus, 300.f, 0.f },
			{ AbilityData::LanceDrive::Attribute::ThrustBonus, 0.25f, 0.f },
			{ AbilityData::LanceDrive::Attribute::TurnCapabilityMultiplier, 0.35f, 0.f },
			{ AbilityData::LanceDrive::Attribute::SameTargetHitCooldown, 0.75f, 0.f },
			{ AbilityData::LanceDrive::Attribute::Length, 146.25f, 0.f },
			{ AbilityData::LanceDrive::Attribute::EdgeThickness, 6.f, 0.f },
			{ AbilityData::LanceDrive::Attribute::OpeningAngleDegrees, 50.f, 0.f },
			{ AbilityData::LanceDrive::Attribute::LateralKnockback, 75.f, 0.f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					{ AbilityData::LanceDrive::Attribute::BaseDamage,
						sas::AttributeModifierOperation::Add, 2.f },
					{ AbilityData::LanceDrive::Attribute::TopSpeedBonus,
						sas::AttributeModifierOperation::Add, 5.f },
					{ AbilityData::LanceDrive::Attribute::SpeedDamageConversion,
						sas::AttributeModifierOperation::Add, 0.01f },
					{ ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add, -0.25f }
				},
				{}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::LanceDrive;
		return definition;
	}();
}
