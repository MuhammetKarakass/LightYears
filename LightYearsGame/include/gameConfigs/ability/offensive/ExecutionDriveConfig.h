#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance is loaded from abilities.json. This fallback only provides
	// the executable family identity and safe values for tests without content.
	inline const ly::GameAbilityDefinition ExecutionDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ExecutionDrive::AbilityId::Basic;
		// Legacy fallback slot; runtime AbilityLoadoutManager may rebind this
		// ability to any player ability slot after acquisition.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ExecutionDrive
		};
		definition.displayName = "Execution Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 80, 35, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus,
				20.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::AttackPowerScale,
				0.10f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus,
				0.15f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::Range,
				700.f,
				1.f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					{ AbilityData::ExecutionDrive::Attribute::FlatAttackPowerBonus,
						sas::AttributeModifierOperation::Add, 2.f },
					{ AbilityData::ExecutionDrive::Attribute::AttackPowerScale,
						sas::AttributeModifierOperation::Add, 0.01f },
					{ AbilityData::ExecutionDrive::Attribute::MoveSpeedBonus,
						sas::AttributeModifierOperation::Add, 0.01f }
				},
				{}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ExecutionDrive;
		return definition;
	}();
}
