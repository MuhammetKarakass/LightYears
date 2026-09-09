#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/inertialWake/InertialWakeContracts.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/inertialWake/InertialWakePresentationIds.h"

namespace AbilityData::InertialWake
{
	// C++ declares only shape/type/presentation ownership. Numeric balance is
	// loaded from abilities.json so design tuning never forks between code/data.
	inline const ly::AbilityActorDefinition ActorWakeBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Wake::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::InertialWake;
		definition.presentationProfileId = ly::InertialWakePresentationIds::WakeBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition InertialWake_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::InertialWake::AbilityId::Basic;
		// Runtime loadouts, not this catalog fallback, own the player's key slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 16.f;
		definition.duration = 8.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::InertialWake
		};
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.displayName = "Inertial Wake";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 105, 225, 255, 255 };
		definition.attributes = {
			{ AbilityData::InertialWake::Attribute::TopSpeedBonus, 100.f, 0.f },
			{ AbilityData::InertialWake::Attribute::ThrustBonus, 0.15f, 0.f },
			{ AbilityData::InertialWake::Attribute::NormalizationDuration, 0.5f, 0.01f }
		};
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{ AbilityData::InertialWake::ActorWakeBasic.actorDefinitionId,
					sas::AbilitySpawnPolicy::AtOwner },
				0.f,
				1
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{ AbilityData::InertialWake::Attribute::TopSpeedBonus,
						sas::AttributeModifierOperation::Add, 5.f },
					sas::AttributeModifier{ AbilityData::InertialWake::Actor::Wake::SpeedDamageConversion,
						sas::AttributeModifierOperation::Add, 0.01f },
					sas::AttributeModifier{ ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add, -0.25f }
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::InertialWake;
		return definition;
	}();
}
