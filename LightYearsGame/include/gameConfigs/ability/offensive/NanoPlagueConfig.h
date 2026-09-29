#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// JSON is authoritative for balance. This fallback supplies only the stable
	// content identity used by behavior registration and content-loader tests.
	inline const ly::GameAbilityDefinition NanoPlague_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::NanoPlague::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::NanoPlague
		};
		definition.displayName = "Nano Plague";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 90, 215, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::NanoPlague;
		return definition;
	}();
}
