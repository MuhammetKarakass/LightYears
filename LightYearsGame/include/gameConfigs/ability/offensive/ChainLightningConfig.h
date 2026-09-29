#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/chainLightning/ChainLightningContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"


namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ChainLightning_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ChainLightning::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// The runtime loadout manager may bind this ability to any Ability1-4
		// slot. Ability1 is only a valid standalone content fallback and does not
		// assign a permanent player key.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ChainLightning
		};
		definition.displayName = "Chain Lightning";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 230, 255, 255 };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		// The chain traversal owns targeting and delayed impact. There must be no
		// generic action here that could apply damage immediately on activation.
		definition.behaviorType = ly::AbilityBehaviorType::ChainLightning;
		return definition;
	}();
}
