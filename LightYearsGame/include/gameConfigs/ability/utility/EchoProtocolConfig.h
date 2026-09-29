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
		// Numeric balance, progression and costs are authored in abilities.json.
		// F is Echo's default binding. The runtime loadout may still rebind it
		// after the player acquires the ability.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::EchoProtocol
		};
		definition.displayName = "Echo Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 120, 210, 255, 255 };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Cooldown
		};
		definition.behaviorType = ly::AbilityBehaviorType::EchoProtocol;
		definition.recordInAbilityHistory = false;
		return definition;
	}();
}
