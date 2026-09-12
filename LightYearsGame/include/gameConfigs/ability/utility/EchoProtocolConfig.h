#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/echoProtocol/EchoProtocolContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition EchoProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EchoProtocol::AbilityId::Basic;
		// F is Echo's default binding. The runtime loadout may still rebind it
		// after the player acquires the ability.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 18.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::EchoProtocol
		};
		definition.displayName = "Echo Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 120, 210, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::PowerBase,
				0.60f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::PowerPerLevel,
				0.03f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::AttackPowerScale,
				1.30f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::MaxHealthScale,
				0.20f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::EnergyPowerScale,
				0.10f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::AttackSpeedScale,
				0.20f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::LuckScale,
				0.25f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::EchoProtocol::Attribute::MovementScale,
				0.15f,
				0.f
			}
		};
		definition.levelProgression.reserve(14);
		for (int index = 0; index < 14; ++index)
		{
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.30f
					}
				},
				{},
				{},
				{}
			});
		}
		definition.levelUpgradeScrapCosts = {
			80, 80, 80, 80, 80, 80, 80,
			80, 80, 80, 80, 80, 80, 80
		};
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Cooldown
		};
		definition.behaviorType = ly::AbilityBehaviorType::EchoProtocol;
		definition.recordInAbilityHistory = false;
		return definition;
	}();
}
