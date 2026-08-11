#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition NullPulse_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::NullPulse::AbilityId::Basic;
		// Ability2 is the default control/active slot and is consumed by the E
		// input in the player loadout.
		definition.slot = sas::AbilitySlot::Ability2;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 0.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Control,
			ly::GameplayTags::Ability::Family::NullPulse
		};
		definition.displayName = "Null Pulse";
		// Use an asset that is shipped in the repository. The old star_blue.png
		// path did not exist and caused the HUD image widget to receive a
		// missing texture during startup.
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.inputLabel = "E";
		definition.accentColor = sf::Color{ 100, 225, 255, 255 };
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Cooldown
		};
		definition.behaviorType = ly::AbilityBehaviorType::NullPulse;
		return definition;
	}();
}
