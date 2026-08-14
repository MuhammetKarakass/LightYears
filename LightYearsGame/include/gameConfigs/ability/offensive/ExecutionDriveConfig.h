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
				AbilityData::ExecutionDrive::Attribute::BaseAttackPowerBonus,
				10.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::AttackPowerPerStack,
				3.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::BaseChaseMovementBonus,
				0.10f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::ChaseMovementPerStack,
				0.02f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::AttackPowerChaseScale,
				0.001f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::TargetingRange,
				700.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::ExecutionDrive::Attribute::DirectionThreshold,
				0.25f,
				-1.f,
				1.f
			}
		};
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::ExecutionDrive;
		return definition;
	}();
}
