#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition NullPulse_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::NullPulse::AbilityId::Basic;
		// Ability2 is the default control/active slot and is consumed by the E
		// input in the player loadout.
		definition.slot = sas::AbilitySlot::Ability2;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Utility,
			vega::GameplayTags::Ability::Family::NullPulse
		};
		definition.displayName = "Null Pulse";
		// Use an asset that is shipped in the repository. The old star_blue.png
		// path did not exist and caused the HUD image widget to receive a
		// missing texture during startup.
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 100, 225, 255, 255 };
		definition.damageTags = { vega::DamageTypeSchema::Energy };
		definition.behaviorType = vega::AbilityBehaviorType::NullPulse;
		return definition;
	}();
}
